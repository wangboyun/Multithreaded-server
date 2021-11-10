/**
 * @file address.cpp
 * @brief  地址类封装实现
 * @author wyz (501826086@qq.com)
 * @version 1.0
 * @date 2021-11-06
 * 
 * @copyright Copyright (c) 2021  wyz
 * 
 */

#include "address.h"
#include "log.h"
#include "mutex.h"
#include "endian.h"

#include <cerrno>
#include <cstdint>
#include <ifaddrs.h>
#include <memory>
#include <netdb.h>
#include <algorithm>
#include <arpa/inet.h>
#include <cstddef>
#include <cstring>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <utility>
#include <vector>


namespace wyz {

static wyz::Logger::ptr g_logger = WYZ_LOG_NAME("system");

/* 创建子网掩码 */
template<class T>
static T CreateMask(uint32_t bits){
    return (1 << (sizeof(T)* 8 - bits)) - 1;
}

/* 计算二进制 1的数量 */
template<class T>
static uint32_t CountoneBytes(T val){
    uint32_t res;
    for(; val ; ++ res){
        val &= val - 1;
    }
    return res;
}

Address::ptr Address::Create(const sockaddr* addr, socklen_t addrlen){
    if(!addr)
        return nullptr;
    switch (addr->sa_family) {
        case AF_INET:
            return IPv4Address::ptr(new IPv4Address(*(const sockaddr_in*)(addr)));
        case AF_INET6 : 
            return IPv6Address::ptr(new IPv6Address(*(const sockaddr_in6*)(addr)));
        default:
            return UnKnowAddress::ptr(new UnKnowAddress(* addr));
    }
}

bool Address::Lookup (std::vector<Address::ptr>& result , const std::string& host , int family , int type , int protocol ){
    addrinfo hints , *results, *next;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = 0;
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    hints.ai_addrlen = 0;
    hints.ai_addr = nullptr;
    hints.ai_canonname = nullptr;
    hints.ai_next = nullptr;

    const char* service = nullptr;
    std::string node;

    if(!host.empty() && host[0] == '['){
        const char* endipv6 = (const char*)memchr(host.c_str()+1,']' , host.size()-1 );
        if(endipv6){
            /// 这里有越界的问题
            if(*(endipv6 +1) == ':'){
                service = endipv6 + 2;
            }
            node = host.substr(1 , endipv6 - host.c_str() -1 );
        }
    }

    //检查 node serivce
    if(node.empty()) {
        service = (const char*)memchr(host.c_str(), ':', host.size());
        if(service) {
            if(!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
                node = host.substr(0, service - host.c_str());
                ++service;
            }
        }
    }

    if(node.empty()) {
        node = host;
    }

    int err = getaddrinfo(node.c_str(), service, &hints, &results);
    if(err){
        WYZ_LOG_ERROR(g_logger) << "Address::Lookup getaddrinfo err=" << err 
                                << "errnum =" << errno << strerror(errno);  
        return false;
    }
    next = results;
    while(next){
        result.emplace_back(Address::Create(next->ai_addr,next->ai_addrlen));
        next = next->ai_next;
    }
    freeaddrinfo(results);
    return !result.empty();
}

Address::ptr Address::LookupAnyAddress(const std::string& host , int family
                                    , int type , int protocol ){
    std::vector<Address::ptr> res;
    if(Lookup(res, host, family, type, protocol)){
        return res[0];
    }
    return nullptr;
}

std::shared_ptr<IPAddress> Address::LookupIPAddress(const std::string& host , int family 
    , int type , int protocol ){
    std::vector<Address::ptr> res;
    if(Lookup(res, host, family, type, protocol)){
        for(auto i : res){
            if(i->getFamily() == AF_INET || i->getFamily() == AF_INET6){
                return std::dynamic_pointer_cast<IPAddress>(i);
            }
        }
    }
    return nullptr;
}

/* 获得本机所有网卡的 <网卡名, 地址, 子网掩码位数>*/
bool Address::GetInterfaceAddresses(std::multimap<std::string
    ,std::pair<Address::ptr, uint32_t> >& result,int family ){
    ifaddrs* res , *next;
    if(getifaddrs(&res)){
        WYZ_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses getifaddrs err";
        return false;
    }
    next = res;
    try {
        while(next){
            Address::ptr addr;
            uint32_t prefix_len  = ~0u;
            if(family != AF_UNSPEC && family != next->ifa_addr->sa_family){
                next = next->ifa_next;
                continue;
            }
            switch (next->ifa_addr->sa_family) {
                case AF_INET:
                    {
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                        uint32_t netmask = ((const sockaddr_in*) next->ifa_netmask)->sin_addr.s_addr;
                        prefix_len = CountoneBytes(netmask);
                    }
                    break;
                case AF_INET6:
                    {
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                        in6_addr netmask = ((const sockaddr_in6*) next->ifa_netmask)->sin6_addr;
                        prefix_len = 0;
                        for(int i = 0 ; i < 16; ++i)
                            prefix_len += CountoneBytes(netmask.s6_addr[i]);
                    }
                    break;
                default:
                    break;
            }
            if(addr){
                result.insert(std::make_pair(next->ifa_name, std::make_pair(addr, prefix_len)));
            }
            next = next->ifa_next;
        }
    } catch (...) {
        WYZ_LOG_ERROR(g_logger) << "Address::GetInterfaceAddresses excepetion";
        freeifaddrs(res);
        return false;
    }
    freeifaddrs(res);
    return !result.empty();
}

bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >&result,const std::string& iface, int family ){
    if(iface.empty() || iface == "*") {
        if(family == AF_INET || family == AF_UNSPEC) {
            result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
        }
        if(family == AF_INET6 || family == AF_UNSPEC) {
            result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
        }
        return true;
    }

    std::multimap<std::string
          ,std::pair<Address::ptr, uint32_t> > results;

    if(!GetInterfaceAddresses(results, family)) {
        return false;
    }

    auto its = results.equal_range(iface);
    for(; its.first != its.second; ++its.first) {
        result.push_back(its.first->second);
    }
    return !result.empty();
}


int Address::getFamily()const{
    return getAddr()->sa_family;
}
std::string Address::toString(){
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

bool Address::operator< (const Address& rhs)const{
    int minlen = std::min(this->getLen() , rhs.getLen());
    int cmp = memcmp(this->getAddr(), rhs.getAddr(), minlen);
    if(cmp < 0)
        return true;
    else if(cmp > 0)
        return false;
    else if(this->getLen() < rhs.getLen())
        return true;
    return false;
}
bool Address::operator== (const Address& rhs)const{
    return this->getLen() == rhs.getLen() 
        && memcmp(this->getAddr(), rhs.getAddr(), rhs.getLen()) == 0;
}
bool Address::operator!= (const Address& rhs)const{
    return !(*this == rhs);
}

IPAddress::ptr IPAddress::Create(const char* address, uint16_t port){
    addrinfo hints , *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_flags = AI_NUMERICHOST;
    int err = getaddrinfo(address, nullptr, &hints, &result);
    if(err){
        WYZ_LOG_ERROR(g_logger) << "IPAddress::Create(" << address
            << ", " << port << ") error=" << err
            << " errno=" << errno << " errstr=" << strerror(errno);
        return nullptr; 
    }
    try {
        IPAddress::ptr res = std::dynamic_pointer_cast<IPAddress>(Address::Create(result->ai_addr
            , (socklen_t) result->ai_addrlen));
        if(res){
            res->setPort(port);
        }
        freeaddrinfo(result);
        return res;
    } catch (...) {
        WYZ_LOG_ERROR(g_logger) <<  "IPAddress::Create dynamic_pointer_cast (" << address << ", " << port << ") error";
        freeaddrinfo(result);
        return nullptr;
    }

}

IPv4Address::ptr IPv4Address::Create(const char* address, uint16_t port){
    IPv4Address::ptr rt(new IPv4Address);
    rt->m_addr.sin_port = htons(port);
    /* 把点分式地址转变为网络地址 */
    int res = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
    if(res <= 0){
        WYZ_LOG_ERROR(g_logger) << "IPv4Address::Create(" << address << ", "<< port << ") rt=" << res << " errno="
        << errno << " errstr=" << strerror(errno); 
        return nullptr;
    }
    return rt;
}

IPv4Address::IPv4Address(const sockaddr_in& address)
    : m_addr(address){}

IPv4Address::IPv4Address(uint32_t address , uint16_t port ){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);   /// 本地字节序转化为网络字节序
    m_addr.sin_addr.s_addr = htonl(address);
}

const sockaddr* IPv4Address::getAddr()const{
    return (sockaddr*)&m_addr;
}
    
const socklen_t IPv4Address::getLen()const {
    return (socklen_t)sizeof(m_addr);
}

std::ostream& IPv4Address::insert(std::ostream& os)const {
    uint32_t addr = ntohl(m_addr.sin_addr.s_addr);
    os  << ((addr >> 24) & 0xff) << "."
        << ((addr >> 16) & 0xff) << "."
        << ((addr >> 8) & 0xff) << "."
        << (addr & 0xff) << ":" << ntohs(m_addr.sin_port);
    return os;
}

/* */
IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
    if(prefix_len > 32)
        return nullptr;
    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr |= htonl(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}

IPAddress::ptr IPv4Address::networdAddress(uint32_t prefix_len){
    if(prefix_len > 32)
        return nullptr;
    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr &= ~htonl(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));

}

IPAddress::ptr IPv4Address::subnetAddress(uint32_t prefix_len){
    if(prefix_len > 32)
        return nullptr;
    sockaddr_in subnet(m_addr);
    subnet.sin_addr.s_addr = ~htonl(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(subnet));
}

uint32_t IPv4Address::getPort()const {
    return ntohs(m_addr.sin_port);
}

void IPv4Address::setPort(uint16_t port) {
    m_addr.sin_port = htons(port);
}

IPv6Address::ptr IPv6Address::Create(const char* address, uint16_t port) {
    IPv6Address::ptr rt(new IPv6Address);
    rt->m_addr.sin6_port = htons(port);
    int res = inet_pton(AF_INET6, address, rt->m_addr.sin6_addr.s6_addr);
    if(res <= 0){
        WYZ_LOG_ERROR(g_logger) << "IPv6Address::Create(" << address << ", "<< port << ") rt=" << res << " errno="
        << errno << " errstr=" << strerror(errno); 
        return nullptr;
    }
    return rt;
}

IPv6Address::IPv6Address(){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const sockaddr_in6& address)
    : m_addr(address){}

IPv6Address::IPv6Address(const uint8_t address[16] , uint16_t port ){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
    m_addr.sin6_port = htons(port);
    memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
}

const sockaddr* IPv6Address::getAddr()const {
    return (sockaddr*)(&m_addr);
}
    
const socklen_t IPv6Address::getLen()const {
    return (socklen_t)sizeof(m_addr);
}

/// 输出IPv6 格式 没看懂要学下 IPv6 的格式 
/// 格式要求：每段中连续的0可省略为"::"，但只能出现一次
std::ostream& IPv6Address::insert(std::ostream& os)const {
    os << "[" ;
    uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
    bool used_zeros = false;
    uint8_t zeroscout = 0;
    for(size_t i = 0 ; i < 8 ; ++i){
        if(addr[i] == 0 && !used_zeros){
            ++zeroscout;
            continue;
        }
        if(i  && addr[i -1] == 0 && !used_zeros ){
            if(zeroscout == 1){
                os << ":0 ";
                zeroscout = 0;
            }else if (zeroscout > 1) {
                os << ":";
                used_zeros = true;
            }
        }
        if(i){
            os << ":";
        }
        os << std::hex << (int) ntohs(addr[i]) << std::dec;
    }
    if(!used_zeros && addr[7] == 0){
        os << "::";
    }

    os << "]:" << ntohs(m_addr.sin6_port);
    return os;
}

/* 修改了子网掩码的起始位置 */
IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
    if(prefix_len > 128){
        return nullptr;
    }
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[ 16 - prefix_len / 8] |= CreateMask<uint8_t>(prefix_len % 8);
    for(size_t i = 16 - prefix_len / 8 + 1 ; i < 16 ; ++i){
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));

}

/* 修改了子网掩码的起始位置 */
IPAddress::ptr IPv6Address::networdAddress(uint32_t prefix_len) {
    if(prefix_len > 128){
        return nullptr;
    }
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[16 -prefix_len / 8] &= CreateMask<uint8_t>(prefix_len % 8);
    for(size_t i = 16 - prefix_len / 8 + 1 ; i < 16 ; ++i){
        baddr.sin6_addr.s6_addr[i] = 0x00;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}

/* 修改了子网掩码的起始位置 */ 
IPAddress::ptr IPv6Address::subnetAddress(uint32_t prefix_len) {
    if(prefix_len > 128){
        return nullptr;
    }
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[16 - prefix_len / 8] = ~CreateMask<uint8_t>(prefix_len % 8);
    for(size_t i = 0 ; i < 16 - prefix_len / 8 ; ++i){
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}

uint32_t IPv6Address::getPort()const {
    return ntohs(m_addr.sin6_port);
}

void IPv6Address::setPort(uint16_t port) {
    m_addr.sin6_port = htons(port);
}

UnixAddress::UnixAddress(const std::string& path){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_len = path.size() + 1;
    if(!path.empty() && path[0] == '\0'){
        --m_len;
    }

    if(m_len > sizeof(m_addr.sun_path)){
        throw std::logic_error(" path len to long");
    }

    memcpy(m_addr.sun_path, path.c_str(), m_len);
    m_len += offsetof(sockaddr_un, sun_path);
}

void UnixAddress::setLen(uint32_t val){
    m_len = val;
}

/* 这一部分也不懂 */
std::string UnixAddress::getpath()const {
    std::stringstream ss;
    if(m_len > offsetof(sockaddr_un, sun_path)
            && m_addr.sun_path[0] == '\0') {
        ss << "\\0" << std::string(m_addr.sun_path + 1,
                m_len - offsetof(sockaddr_un, sun_path) - 1);
    } else {
        ss << m_addr.sun_path;
    }
    return ss.str();
}

const sockaddr* UnixAddress::getAddr()const {
    return (sockaddr*)& m_addr;
}

const socklen_t UnixAddress::getLen()const {
    return m_len;
}
    
std::ostream& UnixAddress::insert(std::ostream& os)const {
    if(m_len > offsetof(sockaddr_un, sun_path)
            && m_addr.sun_path[0] == '\0') {
        return os << "\\0" << std::string(m_addr.sun_path + 1,
                m_len - offsetof(sockaddr_un, sun_path) - 1);
    } else {
        return os << m_addr.sun_path;
    }
}

UnKnowAddress::UnKnowAddress(int family){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}

UnKnowAddress::UnKnowAddress(const sockaddr& address)
    : m_addr(address){}

const sockaddr* UnKnowAddress::getAddr()const {
    return &m_addr;
}
    
const socklen_t UnKnowAddress::getLen()const {
    return sizeof(m_addr);
}
    
std::ostream& UnKnowAddress::insert(std::ostream& os)const {
    return os << "[UnKnowAddress  family=" << m_addr.sa_family;
}

}