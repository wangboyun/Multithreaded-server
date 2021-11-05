/*
 * @Description: 
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-11-05 10:22:33
 */

#include "../src/log.h"
#include "../src/iomanager.h"
#include "../src/address.h"
#include "../src/socket.h"
#include <string>

static wyz::Logger::ptr g_looger = WYZ_LOG_ROOT();

void test_sock(){
    wyz::IPv4Address::ptr addr = std::dynamic_pointer_cast<wyz::IPv4Address>(wyz::Address::LookupIPAddress("www.baidu.com"));

    if(addr) {
        WYZ_LOG_INFO(g_looger) << "get address: " << addr->toString();
    } else {
        WYZ_LOG_INFO(g_looger) << "get address fail";
        return;
    }

    wyz::Socket::ptr sock = wyz::Socket::CreateTCP(addr);
    addr->setPort(80);
    WYZ_LOG_INFO(g_looger) << "get address: " << addr->toString();
    if(!sock->connect(addr)){
        WYZ_LOG_INFO(g_looger) << "sock->connect fail";
        return ;
    }
    WYZ_LOG_INFO(g_looger) << "connect " << addr->toString() << " connected";

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff,sizeof(buff));
    if(rt <= 0) {
        WYZ_LOG_INFO(g_looger) << "send fail rt=" << rt;
        return;
    }
    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());

    if(rt <= 0) {
        WYZ_LOG_INFO(g_looger) << "recv fail rt=" << rt;
        return;
    }

    buffs.resize(rt);
    WYZ_LOG_INFO(g_looger) << buffs;

}

int main(){
    wyz::IOManager iom;
    iom.schedule(&test_sock);
    return 0;
}