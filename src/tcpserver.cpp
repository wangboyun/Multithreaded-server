/**
 * @file tcpserver.cpp
 * @brief Tcp服务器封装
 * @author wyz (501826086@qq.com)
 * @version 1.0
 * @date 2021-11-17
 * 
 * @copyright Copyright (c) 2021  wyz
 * 
 */

#include "tcpserver.h"
#include "log.h"
#include "config.h"
#include <cstring>
#include <functional>
#include <vector>


namespace wyz {

static wyz::Logger::ptr g_logger = WYZ_LOG_NAME("system");

static wyz::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout = Config::Lookup("tcp_server.read_timeout", (uint64_t) (60 * 1000 * 2),"tcp server read timeout");


TCPServer::TCPServer(IOManager* worker, IOManager* acceptworker )
    : m_worker(worker)
    , m_acceptworker(acceptworker)
    , m_recvTimeout(g_tcp_server_read_timeout->getValue())
    , m_name("wyz/1.0.0")
    , m_isStop(true){

}

TCPServer::~TCPServer(){
    for(auto& i : m_socks) {
        i->close();
    }
    m_socks.clear();
}

bool TCPServer::bind(const Address::ptr addr){
    std::vector<Address::ptr> addrs;
    std::vector<Address::ptr> fails;
    addrs.emplace_back(addr);
    return bind(addrs , fails);
}

bool TCPServer::bind(std::vector<Address::ptr>& addrs , std::vector<Address::ptr>& failedaddress){
    bool rt = true;
    for(auto addr : addrs){
        Socket::ptr sock = Socket::CreateTCP(addr);
        if(!sock->bind(addr)){
            /// 没bind 成功
            WYZ_LOG_ERROR(g_logger) << "tcpserver bind errno= " << errno << "strerrno = " << strerror(errno) << " addr=[" << addr->toString() << "]";
            rt = false;
            failedaddress.emplace_back(addr);
            continue;
        }
        if(!sock->listen()){
            WYZ_LOG_ERROR(g_logger) << "tcpserver listen errno= " << errno << "strerrno = " << strerror(errno) << " addr=[" << addr->toString() << "]";
            rt = false;
            failedaddress.emplace_back(addr);
            continue;
        }
        m_socks.emplace_back(sock);
    }
    if(!failedaddress.empty()){
        m_socks.clear();
    }
    for(auto i : m_socks) {
        WYZ_LOG_INFO(g_logger) << " name=" << m_name << " server bind success: " << i->toString();
    }

    return rt;
}

void TCPServer::startAccept(Socket::ptr sock){
    while(!m_isStop){
        Socket::ptr client = sock->accept();
        if(client){
            client->setRecvTimeout(m_recvTimeout);
            m_worker->schedule(std::bind(&TCPServer::handleClient , shared_from_this() , client));
        }else {
            WYZ_LOG_ERROR(g_logger) << "accept errno=" << errno << "errstr= " << strerror(errno);
        }
    }

}

bool TCPServer::start(){
    if(!m_isStop){
        return true;
    }
    m_isStop = false;
    for(auto sock : m_socks){
        m_acceptworker->schedule(std::bind(&TCPServer::startAccept, shared_from_this() , sock));
    }
    return true;
}

void TCPServer::stop(){
    m_isStop = true;
    auto self = shared_from_this();
    m_acceptworker->schedule([this , self]() {
        for(auto sock : m_socks){
            sock->cancelAll();
            sock->close();
        }
        m_socks.clear();
    });
}

void TCPServer::handleClient(Socket::ptr client){
    WYZ_LOG_INFO(g_logger) << "handleClient: " << *client;
}



}