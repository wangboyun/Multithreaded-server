/*
 * @Description: 提供获取线程号与协程号的方法
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-09-23 10:22:34
 */
#ifndef __WYZ__UTIL__H__
#define __WYZ__UTIL__H__

#include <cstdint>
#include <sched.h>
#include <stdint.h>
#include <execinfo.h>
#include <string>
#include <vector>


namespace wyz {
    //获取系统调用的线程号
    pid_t GetThreadId();
    uint32_t GetFiberId();

    void Backtrace(std::vector<std::string>& bt,int size , int skip);

    std::string BacktraceToString(int size = 64 , int skip = 2 , const std::string& prefix = "");

    /* 获取系统当前时间 */
    uint64_t GetCurrentMS();
    uint64_t GetCurrentUS();

}

#endif