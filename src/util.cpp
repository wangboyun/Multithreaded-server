/*
 * @Description: 
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-09-23 10:28:05
 */

#include <sched.h>
#include <string>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#include "util.h"
#include "log.h"
#include "fiber.h"

namespace wyz {
    
    static Logger::ptr g_logger = WYZ_LOG_NAME("system");

    pid_t GetThreadId(){   // 系统线程号
        return (pid_t)syscall(SYS_gettid);
    }
    uint32_t GetFiberId(){     // 等用到协程号时候在重构此代码
        return Fiber::GetFiberId();           
    }

    /* 该函数获取当前线程的调用堆栈，获取的信息将会被存放在bt中，*/
    void Backtrace(std::vector<std::string>& bt,int size , int skip){
        void** buffer = (void **)malloc(sizeof(void*) * size);
        size_t s = backtrace(buffer, size);
        char** strings = backtrace_symbols(buffer, s);
        if(strings == nullptr){
            WYZ_LOG_ERROR(g_logger) << "backtrace_synbols error";
            free(strings);
            free(buffer);
            return;
        }
        for(size_t i = skip; i < s ; ++i){
            bt.emplace_back(strings[i]);
        }

        free(strings);
        free(buffer);
    }

    std::string BacktraceToString(int size , int skip, const std::string& prefix){
        std::vector<std::string> bt;
        Backtrace(bt,size,skip);
        std::stringstream ss;
        for(size_t i = 0 ; i < bt.size() ; ++i){
            ss << prefix << bt[i] << std::endl;
        }
        return ss.str();
    }

    uint64_t GetCurrentMS(){
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return tv.tv_sec * 1000ul + tv.tv_usec / 1000 ;
    }

    uint64_t GetCurrentUS(){
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
    }


}