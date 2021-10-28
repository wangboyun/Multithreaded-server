/*
 * @Description: 
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-09 10:33:08
 */

#ifndef __WYZ_THREAD_H__
#define __WYZ_THREAD_H__


#include <memory>
#include <functional>
#include "mutex.h"

namespace wyz {

/* 线程类 */
class Thread{
public:
    using ptr = std::shared_ptr<Thread>;
    /**
     * @param cb        线程执行函数
     * @param name      线程名字
     */
    Thread(std::function<void ()>cb, const std::string& name);
    ~Thread();

    inline const pid_t getId()const {return m_id;};
    inline const std::string& getName()const {return m_name;};

    void join();

    static Thread* GetThis();
    static const std::string& GetName();            // 此静态方法为日志系统提供获取线程名称
    static void SetName(const std::string& name);   

    static void* run(void* arg);                // 线程运行函数
private:
    Thread(const Thread&) = delete;             // 禁止默认赋值构造函数
    Thread(const Thread&&) = delete;            // 禁止默认移动构造函数
    Thread& operator=(const Thread& ) = delete; // 禁止默认拷贝构造函数

private:
    std::function<void ()> m_cb;                // 线程执行函数
    std::string m_name;                         // 线程名字
    pid_t m_id;                                 // 线程id
    pthread_t m_thread = 0;                     // 创建线程 
    Semaphore m_semaphore;
};

}

#endif