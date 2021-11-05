/*
 * @Description: socket 网络套接字封装
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-11-02 11:17:23
 */

#ifndef __WYZ_SOCKET_H__
#define __WYZ_SOCKET_H__

#include "address.h"

#include "noncopyable.h"
#include <cstdint>
#include <memory>
#include <ostream>
#include <sys/socket.h>


namespace wyz {

class Socket : public std::enable_shared_from_this<Socket> , Noncopyable{
public:
    using ptr = std::shared_ptr<Socket>;
    using weak_ptr = std::weak_ptr<Socket>;

    enum SockType{
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM
    };

    enum SockFamily{
        IPv4 = AF_INET,
        IPv6 = AF_INET6,
        Unix = AF_UNIX
    };

    static Socket::ptr CreateTCP(wyz::Address::ptr address);
    static Socket::ptr CreateUDP(wyz::Address::ptr address);

    /// 创建IPv4 类型的socket
    static Socket::ptr CreateTCPSock();
    static Socket::ptr CreateUDPSock();

    /// 创建IPv6 类型的socket
    static Socket::ptr CreateTCPSock6();
    static Socket::ptr CreateUDPSock6();

    /// 创建unix 类型的TCP/UDP socket
    static Socket::ptr CreateUnixTCPSock();
    static Socket::ptr CreateUnixUDPSock();

    Socket(int family , int type , int protocol = 0);
    ~Socket();

    /* 发送超时相关*/
    int64_t getSendTimeout()const;
    void setSendTimeout(const int64_t timeout_ms);

    /* 接受超时相关 */
    int64_t getRecvTimeout()const;
    void setRecvTimeout(const int64_t timeout_ms);

    /* 套接字选项 */
    bool  getOption(int level , int option , void* val , socklen_t* lenp);
    template<class T>
    bool getOption(int level , int option , T& val){
        socklen_t len = sizeof(T);
        return getOption(level , option , &val, &len);
    }

    bool setOption(int level , int option , const void* val , socklen_t len);
    template<class T>
    bool setOption(int level , int option , const T& val){
        socklen_t len = sizeof(T);
        return setOption(level , option , &val , len);
    }

    /*  服务器接受连接 成功返回一个新的socket 
    *   对这个socket进行读写操作
    */
    Socket::ptr accept();
    bool connect(const Address::ptr address, uint64_t timeout_ms = -1);
    bool bind( const Address::ptr address);
    bool listen(int backlog);
    bool close();

    /// 发送数据部分
    int send(const void * buf, size_t len, int flags = 0);
    int send(const iovec * buf, size_t len, int flags = 0);
    int sendto(const void * buf, size_t len, Address::ptr to, int flags = 0);
    int sendto(const iovec * buf, size_t len, Address::ptr to, int flags = 0);

    /// 接受数据部分
    int recv(void *buf, size_t len, int flags = 0);
    int recv(iovec *buf, size_t len, int flags = 0);
    int recvfrom(void *buf, size_t len, Address::ptr from ,int flags = 0);
    int recvfrom(iovec *buf, size_t len, Address::ptr from ,int flags = 0);

    
    /* 获取远程地址*/
    Address::ptr getRemoteAddress();
    /* 获取本地地址 */
    Address::ptr getLocalAddress();

    /* 连接是否成功 */
    inline bool isConnected()const  {return m_isConnected;}
    /* socket 是否有效*/
    bool isvaild()const;
    int getError();

    std::ostream& dump(std::ostream& os)const;
    std::string toString()const;

    bool cancelRead();
    bool cancelWrite();
    bool cancelAccept();
    bool cancelAll();

    /// 一系列的get函数
    inline const int getSocket()const   {return m_sock;}
    inline const int getFamily() const  {return m_family;}
    inline const int getType() const    {return m_type;}
    inline const int getProtocol() const {return m_protocol;}
protected:
    void initSocket();
    void newSocket();
    bool init(int sock);
private:
    int m_sock;
    int m_family;
    int m_type;
    int m_protocol;
    bool m_isConnected;

    Address::ptr m_remoteAddress;
    Address::ptr m_localAddress;
};

}


#endif