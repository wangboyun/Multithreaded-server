/*
 * @Description: socket 网络套接字模块
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-11-02 11:16:33
 */

#include "socket.h"
#include "fdmanager.h"
#include "log.h"
#include "macro.h"
#include "hook.h"
#include "iomanager.h"
#include <cstdint>
#include <cstring>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

namespace wyz {

static wyz::Logger::ptr g_logger = WYZ_LOG_NAME("system");

Socket::ptr Socket::CreateTCP(wyz::Address::ptr address){
    Socket::ptr sock(new Socket(address->getFamily(), wyz::Socket::TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDP(wyz::Address::ptr address){
    Socket::ptr sock(new Socket(address->getFamily(), wyz::Socket::UDP, 0));
    return sock;
}

Socket::ptr Socket::CreateTCPSock(){
    Socket::ptr sock(new Socket(wyz::Socket::IPv4, wyz::Socket::TCP, 0));
    return sock;
}
Socket::ptr Socket::CreateUDPSock(){
    Socket::ptr sock(new Socket(wyz::Socket::IPv4, wyz::Socket::UDP, 0));
    sock->newSocket();
    sock->m_isConnected = true;
    return sock;
}

Socket::ptr Socket::CreateTCPSock6(){
    Socket::ptr sock(new Socket(wyz::Socket::IPv6, wyz::Socket::TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDPSock6(){
    Socket::ptr sock(new Socket(wyz::Socket::IPv6, wyz::Socket::UDP, 0));
    sock->newSocket();
    sock->m_isConnected = true;
    return sock;
}

Socket::ptr Socket::CreateUnixTCPSock(){
    Socket::ptr sock(new Socket(wyz::Socket::Unix, wyz::Socket::TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUnixUDPSock(){
    Socket::ptr sock(new Socket(wyz::Socket::Unix, wyz::Socket::UDP, 0));
    return sock;
}

Socket::Socket(int family , int type , int protocol)
    : m_sock(-1)
    , m_family(family)
    , m_type(type)
    , m_protocol(protocol)
    , m_isConnected(false){

}
Socket::~Socket(){
    close();
}

    /* 发送超时相关*/
int64_t Socket::getSendTimeout()const{
    FdCtx::ptr ctx = FdMar::GetInstance()->get(m_sock);
    if(ctx){
        return ctx->getTimeout(SO_SNDTIMEO);
    }
    return -1;
}

void Socket::setSendTimeout(const int64_t timeout_ms){
    timeval tv;
    tv.tv_sec = timeout_ms /1000;
    tv.tv_usec = timeout_ms % 1000 * 1000;
    setOption(SOL_SOCKET , SO_SNDTIMEO , tv);
}

    /* 接受超时相关 */
int64_t Socket::getRecvTimeout()const{
    FdCtx::ptr ctx = FdMar::GetInstance()->get(m_sock);
    if(ctx){
        return ctx->getTimeout(SO_RCVTIMEO);
    }
    return -1;
}

void Socket::setRecvTimeout(const int64_t timeout_ms){
    timeval tv;
    tv.tv_sec = timeout_ms /1000;
    tv.tv_usec = timeout_ms % 1000 * 1000;
    setOption(SOL_SOCKET , SO_RCVTIMEO , tv);
}

    /* 套接字选项 */
bool  Socket::getOption(int level , int option , void* val , socklen_t* lenp){
    if(int err = getsockopt(m_sock, level, option, val, lenp)){
        WYZ_LOG_ERROR(g_logger) << " Socket::getOption err = " << err << " strerr" << strerror(errno) << " sock=" << m_sock << " level=" << level << " option" << option;
        return false;
    }
    return true;
}

bool Socket::setOption(int level , int option , const void* val , socklen_t len){
    if(int err = setsockopt(m_sock, level, option, val, len)){
        WYZ_LOG_ERROR(g_logger) << " Socket::setOption err = " << err << " strerr" << strerror(errno) << " sock=" << m_sock << " level=" << level << " option" << option;
        return false;
    }
    return true;
}

Socket::ptr Socket::accept(){
    Socket::ptr sock (new Socket(m_family,m_type,m_protocol));
    int newsock = ::accept(m_sock, nullptr,nullptr);
    if(newsock == -1){
        WYZ_LOG_ERROR(g_logger) << "Socket::accept error sockfd=" << newsock << " errno=" << errno << " strerr=" << strerror(errno);
        return nullptr; 
    }
    if(sock->init(newsock)){
        return sock;
    }
    return nullptr;
}

bool Socket::init(int sock){
    FdCtx::ptr ctx = FdMar::GetInstance()->get(sock);
    if(ctx && ctx->isSocket() && !ctx->isClosed()){
        m_sock = sock;
        m_isConnected = true;
        initSocket();
        getRemoteAddress();
        getLocalAddress();
        return true;
    }
    return false;
}


void Socket::initSocket(){
    int val = 1;
    setOption(SOL_SOCKET,SO_REUSEADDR,val);
    if(m_type == SOCK_STREAM){
        setOption(IPPROTO_TCP,TCP_NODELAY,val);
    }
}

void Socket::newSocket(){
    m_sock = socket(m_family, m_type, m_protocol);
    if(LIKELY(m_sock != -1)){
       initSocket();
    }else {
        WYZ_LOG_ERROR(g_logger) << "socket(" << m_family
            << ", " << m_type << ", " << m_protocol << ") errno="
            << errno << " errstr=" << strerror(errno);
    }
}

bool Socket::bind( const Address::ptr address){
    if(!isvaild()){
        newSocket();
        if(UNLIKELY(!isvaild())){
            return false;
        }
    }
    if(UNLIKELY(address->getFamily() != m_family)) {
        WYZ_LOG_ERROR(g_logger) << "bind sock.family("
            << m_family << ") addr.family(" << address->getFamily()<< ") not equal, addr=" << address->toString();
        return false;
    }

    if(::bind(m_sock, address->getAddr(), address->getLen())){
        WYZ_LOG_ERROR(g_logger) << "Socket::bind(" << m_sock << ") errno=" << errno << " strerr" << strerror(errno);
        return false;
    }
    getLocalAddress();
    return true;
}

bool Socket::connect(const Address::ptr address, uint64_t timeout_ms){
    if(!isvaild()){
        newSocket();
        if(UNLIKELY(!isvaild())){
            return false;
        }
    }
    if(UNLIKELY(address->getFamily() != m_family)) {
        WYZ_LOG_ERROR(g_logger) << "bind sock.family("
            << m_family << ") addr.family(" << address->getFamily()<< ") not equal, addr=" << address->toString();
        return false;
    }

    if(timeout_ms == (uint64_t)-1){
        /// 未设置定时时间
        if(::connect(m_sock, address->getAddr(), address->getLen())){
            WYZ_LOG_ERROR(g_logger) << "Socket::connect(" << m_sock << ") errno=" << errno << " strerr" << strerror(errno);
            close();
            return false;
        }
    }else {
        /// 设置定时时间
        if(::connect_with_timeout(m_sock, address->getAddr(), address->getLen() ,timeout_ms)){
            WYZ_LOG_ERROR(g_logger) << "Socket::connect(" << m_sock << ") errno=" << errno << " strerr" << strerror(errno);
            close();
            return false;
        }
    }
    m_isConnected = true;
    getLocalAddress();
    getRemoteAddress();
    return true;
}

bool Socket::listen(int backlog){
    if(!isvaild()){
        WYZ_LOG_ERROR(g_logger) << "Socket::listen error sockfd= " << m_sock ;
        return false;
    }
    if(::listen(m_sock, backlog)){
        WYZ_LOG_ERROR(g_logger) << "Socket::listen(" << m_sock << ") errno=" << errno << " strerr" << strerror(errno);
        return false;
    }
    return true;
}


bool Socket::close(){
    if(!isvaild() && !m_isConnected){
        return true;
    }
    m_isConnected = false;
    if(isvaild()){
        ::close(m_sock);
        m_sock = -1;
    }
    return false;
}

int Socket::send(const void * buf, size_t len, int flags){
    if(isConnected()){
        return ::send(m_sock, buf, len, flags);
    }
    return -1;
}

int Socket::send(const iovec * buf, size_t len, int flags){
    if(isConnected()){
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec *)buf;
        msg.msg_iovlen = len;
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::sendto(const void * buf, size_t len, Address::ptr to, int flags ){
    if(isConnected()){
        return ::sendto(m_sock, buf, len, flags, to->getAddr(), to->getLen());
    }
    return -1;
}

int Socket::sendto(const iovec * buf, size_t len, Address::ptr to, int flags){
    if(isConnected()){
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec *)buf;
        msg.msg_iovlen = len;
        msg.msg_name = (void *)to->getAddr();
        msg.msg_namelen = to->getLen();
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

    /// 接受数据部分
int Socket::recv(void *buf, size_t len, int flags){
    if(isConnected()){
        return ::recv(m_sock, buf, len, flags);
    }
    return -1;
}

int Socket::recv(iovec *buf, size_t len, int flags ){
    if(isConnected()){
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec *)buf;
        msg.msg_iovlen = len;
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::recvfrom(void *buf, size_t len, Address::ptr from ,int flags ){
    if(isConnected()){
        socklen_t len = from->getLen();
        return ::recvfrom(m_sock, buf, len, flags, (sockaddr *)(from->getAddr()),&len);
    }
    return -1;
}

int Socket::recvfrom(iovec *buf, size_t len, Address::ptr from ,int flags ){
    if(isConnected()){
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec *)buf;
        msg.msg_iovlen = len;
        msg.msg_name = (void *)from->getAddr();
        msg.msg_namelen = from->getLen();
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

bool Socket::isvaild()const{
    return m_sock != -1 ;
}

/* 获取远程地址*/
Address::ptr Socket::getRemoteAddress(){
    if(!isvaild()){
        WYZ_LOG_ERROR(g_logger) << "Socket::getRemoteAddress error sockfd == -1";
        return nullptr;
    }
    if(m_remoteAddress){
        return m_remoteAddress;
    }

    Address::ptr tempaddr;
    switch (m_family) {
        case AF_INET:
            tempaddr.reset(new IPv4Address());
            break;
        case AF_INET6:
            tempaddr.reset(new IPv6Address());
            break;
        case AF_UNIX:
            tempaddr.reset(new UnixAddress());
            break;
        default:
            tempaddr.reset(new UnKnowAddress(m_family));
            break;
    }
    socklen_t len = tempaddr->getLen();
    if(getpeername(m_sock, (sockaddr*)tempaddr->getAddr(), &len)){
        return Address::ptr(new UnKnowAddress(m_family));
    }
    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(tempaddr);
        addr->setLen(len);
    }
    m_remoteAddress = tempaddr;
    return m_remoteAddress;
    
}

/* 获取本地地址 */
Address::ptr Socket::getLocalAddress(){
    if(!isvaild()){
        WYZ_LOG_ERROR(g_logger) << "Socket::getRemoteAddress error sockfd == -1";
        return nullptr;
    }
    if(m_localAddress){
        return m_localAddress;
    }

    Address::ptr tempaddr;
    switch (m_family) {
        case AF_INET:
            tempaddr.reset(new IPv4Address());
            break;
        case AF_INET6:
            tempaddr.reset(new IPv6Address());
            break;
        case AF_UNIX:
            tempaddr.reset(new UnixAddress());
            break;
        default:
            tempaddr.reset(new UnKnowAddress(m_family));
            break;
    }
    socklen_t len = tempaddr->getLen();
    if(getsockname(m_sock, (sockaddr*)tempaddr->getAddr(), &len)){
        return Address::ptr(new UnKnowAddress(m_family));
    }
    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(tempaddr);
        addr->setLen(len);
    }
    m_localAddress = tempaddr;
    return m_localAddress;
}

int Socket::getError(){
	int error = 0;
    socklen_t len = sizeof(error);
    if(!getOption(SOL_SOCKET,SO_ERROR, &error, &len)){
        error = errno;
    }
    return error;
}

std::ostream& Socket::dump(std::ostream& os)const{
    os << "[Socket sock=" << m_sock
       << " is_connected=" << m_isConnected
       << " family=" << m_family
       << " type=" << m_type
       << " protocol=" << m_protocol;
    if(m_localAddress) {
        os << " local_address=" << m_localAddress->toString();
    }
    if(m_remoteAddress) {
        os << " remote_address=" << m_remoteAddress->toString();
    }
    os << "]";
    return os;
}

std::string Socket::toString()const{
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

bool Socket::cancelRead(){
    return IOManager::GetThis()->cancelEvent(m_sock, wyz::IOManager::READ);
}

bool Socket::cancelWrite(){
    return IOManager::GetThis()->cancelEvent(m_sock, wyz::IOManager::WRITE);
}

bool Socket::cancelAccept(){
    return IOManager::GetThis()->cancelEvent(m_sock, wyz::IOManager::READ);
}

bool Socket::cancelAll(){
    return IOManager::GetThis()->cancelEvent(m_sock, wyz::IOManager::READ);

}



}
