/*
 * @Description: 测试协程demo
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-13 11:16:52
 */

#include "../src/log.h"
#include "../src/fiber.h"
#include <string>
#include <vector>

static wyz::Logger::ptr g_logger = WYZ_LOG_ROOT();
void run_fiber(){
    WYZ_LOG_INFO(g_logger) << " run_fiber begin";
    wyz::Fiber::YieldToHold();
    WYZ_LOG_INFO(g_logger) << "run_in_fiber end";
    wyz::Fiber::YieldToHold();

}
void test_fiber(){
    WYZ_LOG_INFO(g_logger) << "main begin - 1";
    {
        wyz::Fiber::GetThis();
        WYZ_LOG_INFO(g_logger) << "main begin";
        wyz::Fiber::ptr fiber(new wyz::Fiber(run_fiber));
        fiber->call();
        WYZ_LOG_INFO(g_logger) << "main after fiber";
        fiber->call();
        WYZ_LOG_INFO(g_logger) << "main end";
        fiber->call();
    }
    WYZ_LOG_INFO(g_logger) << "main end 2";
}

int main(int argc , char** argv){
    wyz::Thread::SetName("main");
    std::vector<wyz::Thread::ptr> thrs;
    for(int i = 0 ; i < 1 ; ++i){
        wyz::Thread::ptr thr(new wyz::Thread(test_fiber,"name_" + std::to_string(i)));
        thrs.emplace_back(thr);
    }
    for(auto i : thrs){
        i->join();
    }
    return 0;
}