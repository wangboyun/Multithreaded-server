/*
 * @Description: 自定义宏模块封装
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-12 10:21:48
 */

#ifndef __WYZ_MACRO_H__
#define __WYZ_MACRO_H__

#include <assert.h>
#include "util.h"

#if defined __GNUC__ || defined __llvm__
/// LIKELY 宏的封装, 告诉编译器优化,条件大概率成立
#define LIKELY(x)       __builtin_expect(!!(x), 1)
/// UNLIKELY 宏的封装, 告诉编译器优化,条件大概率不成立
#define UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#define LIKELY(x)        (x)
#define UNLIKELY(x)      (x)
#endif



// 断言宏封装
#define WYZ_ASSERT(args) \
    if(UNLIKELY(!(args))){ \
        WYZ_LOG_INFO(WYZ_LOG_ROOT()) << "ASSERTION: " #args \
        << "\nbacktrace:\n"  \
        << wyz::BacktraceToString(100 , 2 , "   "); \
        assert(args);\
    }

// 断言宏封装
#define WYZ_ASSERT2(args , desc) \
    if(UNLIKELY(!(args))){ \
        WYZ_LOG_INFO(WYZ_LOG_ROOT()) << "ASSERTION: " #args \
        << "\n" << desc\
        << "\nbacktrace:\n" \
        << wyz::BacktraceToString(100 , 2 , "   ");\
        assert(args);\
    }

#endif