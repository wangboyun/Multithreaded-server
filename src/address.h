/*
 * @Description: socket 网络地址封装 
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-29 14:48:51
 */

#ifndef __WYZ_ADDRESS_H__
#define __WYZ_ADDRESS_H__


#include <memory>
#include <netinet/in.h>
#include <ostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>
#include <map>

namespace wyz {

class IPAddress;

/* 地址基类 */
class Address{

public:
    using ptr = std::shared_ptr<Address>;

     /**
     * @brief 通过sockaddr指针创建Address
     * @param[in] addr sockaddr指针
     * @param[in] addrlen sockaddr的长度
     * @return 返回和sockaddr相匹配的Address,失败返回nullptr
     */
    static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);

    /**
     * @brief 通过host地址返回对应条件的所有Address
     * @param[out] result 保存满足条件的Address
     * @param[in] host 域名,服务器名等.举例: www.sylar.top[:80] (方括号为可选内容)
     * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
     * @param[in] type socketl类型SOCK_STREAM、SOCK_DGRAM 等
     * @param[in] protocol 协议,IPPROTO_TCP、IPPROTO_UDP 等
     * @return 返回是否转换成功
     */ 
    static bool Lookup (std::vector<Address::ptr>& result , const std::string& host , int family = AF_INET , int type = 0 , int protocol = 0);

    static Address::ptr LookupAddress(const std::string& host , int family = AF_INET , int type = 0 , int protocol = 0);

    static std::shared_ptr<IPAddress> LookupIPAddress(const std::string& host , int family = AF_INET , int type = 0 , int protocol = 0);

/**
     * @brief 返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
     * @param[out] result 保存本机所有地址
     * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
     * @return 是否获取成功
     */
    static bool GetInterfaceAddresses(std::multimap<std::string,std::pair<Address::ptr, uint32_t> >& result,int family = AF_INET);
    /**
     * @brief 获取指定网卡的地址和子网掩码位数
     * @param[out] result 保存指定网卡所有地址
     * @param[in] iface 网卡名称
     * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
     * @return 是否获取成功
     */
    static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >&result,const std::string& iface, int family = AF_INET);

    virtual ~Address(){};

    int getFamily()const;
    /* 获取地址*/
    virtual const sockaddr* getAddr()const = 0;
    /* 获取地址长度 */
    virtual const socklen_t getLen()const = 0;
    /* 序列化输出 */
    virtual std::ostream& insert(std::ostream& os)const = 0;

    std::string toString();

    bool operator< (const Address& rhs)const;
    bool operator== (const Address& rhs)const;
    bool operator!= (const Address& rhs)const;

};

/* IP 地址基类 */
class IPAddress : public Address{
public:
    using ptr = std::shared_ptr<IPAddress>;

    /**
     * @brief 通过sockaddr指针创建Address
     * @param[in] addr sockaddr指针
     * @param[in] addrlen sockaddr的长度
     * @return 返回和sockaddr相匹配的Address,失败返回nullptr
     */
    static IPAddress::ptr Create(const char* address, uint16_t port = 0);

    virtual ~IPAddress(){};
    /* 获得广播地址 */
    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0 ;
    /* 获取网关地址 */
    virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0;
    /* 获取子网掩码 */
    virtual IPAddress::ptr subnetAddress(uint32_t prefix_len) = 0;
    /* 获取端口号 */
    virtual uint32_t getPort()const = 0;
    /* 设置端口号 */
    virtual void setPort(uint16_t port) = 0;

};

/* IPv4 地址*/
class IPv4Address : public IPAddress{
public:
    using ptr = std::shared_ptr<IPv4Address>;
    /**
     * @brief 使用点分十进制地址创建IPv4Address
     * @param[in] address 点分十进制地址,如:192.168.1.1
     * @param[in] port 端口号
     * @return 返回IPv4Address,失败返回nullptr
     */
    static IPv4Address::ptr Create(const char* address, uint16_t port = 0);

    IPv4Address(const sockaddr_in& address);
    IPv4Address(uint32_t address = INADDR_ANY , uint16_t port = 0);

    const sockaddr* getAddr()const override;
    /* 获取地址长度 */
    const socklen_t getLen()const override;
    /* 序列化输出 */
    std::ostream& insert(std::ostream& os)const override;

    /**
     * @brief 获取该地址的广播地址
     * @param[in] prefix_len 子网掩码位数
     * @return 调用成功返回IPAddress,失败返回nullptr
     */
    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override ;
    
    /**
     * @brief:  获取该地址的网段
     * @param {uint32_t} prefix_len 子网掩码位数
     * @return 调用成功返回IPAddress,失败返回nullptr
     */ 
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
 
    /**
     * @brief: 获取子网掩码
     * @param {uint32_t} prefix_len   子网掩码位数
     * @return 调用成功返回IPAddress,失败返回nullptr
     */   
    IPAddress::ptr subnetAddress(uint32_t prefix_len) override;
    /* 获取端口号 */
    uint32_t getPort()const override;
    /* 设置端口号 */
    void setPort(uint16_t port) override;
private:
    sockaddr_in m_addr;
};

/* IPv6 地址*/
class IPv6Address : public IPAddress{
public:
    using ptr = std::shared_ptr<IPv6Address>;

    /**
     * @brief 使用点分十进制地址创建IPv6Address
     * @param[in] address IPv6地址字符串
     * @param[in] port 端口号
     * @return 返回IPv6Address,失败返回nullptr
     */
    static IPv6Address::ptr Create(const char* address, uint16_t port = 0);
    IPv6Address();
    IPv6Address(const uint8_t address[16] , uint16_t port = 0 );
    IPv6Address(const sockaddr_in6& address);
    const sockaddr* getAddr()const override;
    /* 获取地址长度 */
    const socklen_t getLen()const override;
    /* 序列化输出 */
    std::ostream& insert(std::ostream& os)const override;
     /**
     * @brief 获取该地址的广播地址
     * @param[in] prefix_len 子网掩码位数
     * @return 调用成功返回IPAddress,失败返回nullptr
     */
    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override ;
   
    /**
     * @brief:  获取网关地址 
     * @param {uint32_t} prefix_len 子网掩码位数
     * @return 调用成功返回IPAddress,失败返回nullptr
     */    
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
   
    /**
     * @brief: 获取子网掩码
     * @param {uint32_t} prefix_len   子网掩码位数
     * @return 调用成功返回IPAddress,失败返回nullptr
     */    
    IPAddress::ptr subnetAddress(uint32_t prefix_len) override;
    /* 获取端口号 */
    uint32_t getPort()const override;
    /* 设置端口号 */
    void setPort(uint16_t port) override;
private:
    sockaddr_in6 m_addr;
};

/* unix 地址 */
class UnixAddress : public Address{
public:
    using ptr = std::shared_ptr<UnixAddress>;
    UnixAddress(){};
    UnixAddress(const std::string& path);

    void setLen(uint32_t val);
    std::string getpath()const ;
    const sockaddr* getAddr()const override;
    /* 获取地址长度 */
    const socklen_t getLen()const override;
    /* 序列化输出 */
    std::ostream& insert(std::ostream& os)const override;

private:
    sockaddr_un m_addr;
    socklen_t m_len;
};


/* 未知类型 */
class UnKnowAddress : public Address{
public:
    using ptr = std::shared_ptr<UnKnowAddress>;

    UnKnowAddress(int family);
    UnKnowAddress(const sockaddr& address);
    const sockaddr* getAddr()const override;
    /* 获取地址长度 */
    const socklen_t getLen()const override;
    /* 序列化输出 */
    std::ostream& insert(std::ostream& os)const override;

private:
    sockaddr m_addr;
};

}


#endif