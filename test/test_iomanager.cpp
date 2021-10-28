/*
 * @Description: 
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-21 17:13:51
 */

#include "../src/log.h"
#include "../src/fiber.h"
#include "../src/scheduler.h"
#include "../src/iomanager.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

static wyz::Logger::ptr g_logger = WYZ_LOG_ROOT();
int sock = 0;
void test_fiber(){
    WYZ_LOG_INFO(g_logger) << "test fiber socket";

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET , "220.181.38.251", &addr.sin_addr.s_addr);

    if(!connect(sock, (const struct sockaddr *) &addr, sizeof(addr))){
    }else if(errno == EINPROGRESS) {
        WYZ_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        wyz::IOManager::GetThis()->addEvent(sock, wyz::IOManager::READ, [](){
            WYZ_LOG_INFO(g_logger) << "read callback";
        });
        wyz::IOManager::GetThis()->addEvent(sock, wyz::IOManager::WRITE, [](){
            WYZ_LOG_INFO(g_logger) << "write callback";
            wyz::IOManager::GetThis()->cancelEvent(sock, wyz::IOManager::READ);
            close(sock);
        });
    } else {
        WYZ_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}

void test(){
    wyz::IOManager iom(2,false , "test");
    iom.schedule(&test_fiber);
}

wyz::Timer::ptr timer;
void test_timer(){
    wyz::IOManager iom(2, true , "test");
    static int i = 0;
    timer = iom.addTimer(1000, [](){
        WYZ_LOG_INFO(g_logger) << "addtimer i = " << i;
        if(++i > 3){
            timer->reset(2000,true); 
            
        }
        if(i == 6){
            timer->cancel();
        }
    } , true);
}

int main(){
    //test();
    test_timer();
    return 0;
}