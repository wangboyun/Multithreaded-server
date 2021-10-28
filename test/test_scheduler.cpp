/*
 * @Description: 测试协程控制模块
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-15 11:06:01
 */

#include "../src/log.h"
#include "../src/scheduler.h"
#include "../src/fiber.h"
#include <unistd.h>

wyz::Logger::ptr g_logger = WYZ_LOG_ROOT();
static int s_count = 5;
void test_task(){
   
    WYZ_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;

    // sleep(1);
    if(--s_count >= 0) {
        wyz::Scheduler::GetThis()->schedule(&test_task,wyz::GetThreadId());
        WYZ_LOG_INFO(g_logger) << "schedule";
        // sleep(2);
    }
}

void test_sche(){
    WYZ_LOG_INFO(g_logger) << "main ";
    wyz::Scheduler sc(3 ,false , "test");
    sc.start();
    sleep(2);
    WYZ_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_task);
    sc.stop();
    WYZ_LOG_INFO(g_logger) << "over ";
}

int main(int argc , char** argv){
    test_sche();
    return 0;
}