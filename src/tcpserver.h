/**
 * @file tcpserver.h
 * @brief tcp 服务端封装
 * @author wyz (501826086@qq.com)
 * @version 1.0
 * @date 2021-11-17
 * 
 * @copyright Copyright (c) 2021  wyz
 * 
 */

#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include <memory>
#include <functional>
#include "iomanager.h"
#include "address.h"
#include "socket.h"
#include "noncopyable.h"

namespace wyz {

class TCPServer : public std::enable_shared_from_this<TCPServer>
                , Noncopyable {
public:
    using ptr = std::shared_ptr<TCPServer>;
    TCPServer(IOManager* worker = IOManager::GetThis(), IOManager* acceptworker = IOManager::GetThis());
    virtual ~TCPServer();
    virtual bool bind(const Address::ptr addr);
    virtual bool bind(std::vector<Address::ptr>& addrs , std::vector<Address::ptr>& failedaddress);

    virtual bool start();
    virtual void stop();

    inline uint64_t getReadTimeout()const       {return m_recvTimeout;}
    inline std::string getName()    const       {return m_name;}

    inline void setReadTimeout(uint64_t v)      {m_recvTimeout = v;}
    inline void setName(const std::string& v)   {m_name = v;}

    inline bool isStop()    const               {return m_isStop;}
protected:
    virtual void handleClient(Socket::ptr client);            /// 服务器连接上一个 socket 后触发的回调
    virtual void startAccept(Socket::ptr sock);             /// 接受客户端连接
private:
    std::vector<Socket::ptr> m_socks;       /// 存放已经accpect 的socket
    IOManager* m_worker;                    /// 主工作线程 
    IOManager* m_acceptworker;              /// (每接受一个socket，突发一个回调函数，m_acceptworker 调度下)
    uint64_t m_recvTimeout;                 /// 服务器接受数据超时时间
    std::string m_name;                     /// tcpserver name
    bool m_isStop;                          /// tcpserver 是否停止
};

}

#endif