/*
 * @Description: 测试hook
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-25 14:12:24
 */

#include "../src/hook.h"
#include "../src/iomanager.h"
#include "../src/log.h"
#include "../src/fiber.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/types.h>          
#include <sys/socket.h>

static wyz::Logger::ptr g_logger = WYZ_LOG_ROOT();

void test_time(){
    wyz::IOManager iom(1);
    iom.schedule([](){
        sleep(2);
        WYZ_LOG_INFO(g_logger) << " sleep 2" ;
    });

    iom.schedule([](){
        usleep(3000000);
        WYZ_LOG_INFO(g_logger) << " sleep 3" ;
    });

    WYZ_LOG_INFO(g_logger) << " test sleep " ;
}

void test_socket(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET , "220.181.38.251", &addr.sin_addr.s_addr);

    WYZ_LOG_INFO(g_logger) << "begin connect";
    int rt = connect(sock, (const struct sockaddr *) &addr, sizeof(addr));
    WYZ_LOG_INFO(g_logger) << "rt= "<<rt << "errno= " << errno; 
    if(rt){
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock, data, sizeof(data), 0);
    WYZ_LOG_INFO(g_logger) << "send rt= " << rt << "  errno= " << errno;

    if(rt <= 0) {
        return;
    }

    std::string buff;
    buff.resize(4096);

    rt = recv(sock, &buff[0], buff.size(), 0);
    WYZ_LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;

    if(rt <= 0) {
        return;
    }

    buff.resize(rt);
    WYZ_LOG_INFO(g_logger) << buff;

}


int main(int argc , char** argv){
    // test_time();
    // test_socket();
    wyz::IOManager iom;
    iom.schedule(test_socket);
    return 0;
}