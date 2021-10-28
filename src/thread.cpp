/*
 * @Description: 
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-09 10:33:22
 */

#include <cstdint>
#include <pthread.h>
#include <semaphore.h>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include "log.h"
#include "util.h"
#include "thread.h"



namespace wyz {
static wyz::Logger::ptr s_logger = WYZ_LOG_NAME("system");

/* thread_local 为线程内静态变量 生命周期为线程周期 */
static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";

Thread* Thread::GetThis(){
    return t_thread;
}
const std::string& Thread::GetName(){
    return t_thread_name;
}
void Thread::SetName(const std::string& name){
    if(name.empty())
        return ;
    if(t_thread){
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

void* Thread::run(void* arg){       // arg 传入的就是this
    Thread* thread = static_cast<Thread*>(arg);
    t_thread = thread;
    t_thread_name = thread->m_name;
    thread->m_id = wyz::GetThreadId();

    /*功能值：该pthread_setname_np（）函数设置目标线程的名称。由名称指定的缓冲区必须包含一个长度为15个字符或更少的空终止字符串（不包括NULL）。如果名称长度超过15个字符，则多余的字符将被忽略。如果name为null，则该线程变为未命名
    返回值：成功返回0*/
    pthread_setname_np(pthread_self(), thread->m_name.substr(0,15).c_str());

    std::function<void ()> cb;
    cb.swap(thread->m_cb);          // 交换cb 与 this->cb

    thread->m_semaphore.post();
    // sleep(1);
    cb();

    return nullptr;
}                // 线程运行函数


Thread::Thread(std::function<void ()>cb, const std::string& name)
: m_cb(cb)
, m_name(name){
    if(name.empty())
        m_name = "UNKNOW";
    
    int err = pthread_create(&m_thread, nullptr, Thread::run, this);
    if(err){
        // 错误日志打印 -- 统一在系统日志中打印
        WYZ_LOG_ERROR(s_logger) << "pthread creat fail name="   << m_name << "pthread id=" << GetThis()->getId();
        throw std::logic_error("pthread creat error");
    }

    m_semaphore.wait();            // 等待信号量值不是0 ，， 阻塞
}
Thread::~Thread(){
    if(m_thread){
        pthread_detach(m_thread);               // 将处于分离状态的线程
    }
}

void Thread::join(){
    if(m_thread){
        int err = pthread_join(m_thread, nullptr);
        if(err){
        // 错误日志打印 -- 统一在系统日志中打印
            WYZ_LOG_ERROR(s_logger) << "pthread join fail name="   << m_name << "pthread id=" << GetThis()->getId();
            throw std::logic_error("pthread join error");
        }
        m_thread = 0;
    }
}


Semaphore::Semaphore(const unsigned int val ){
    if(sem_init(&m_sem, 0, val)){
        WYZ_LOG_ERROR(s_logger) << "sem_init fail";
        throw std::logic_error("sem_init error");
    }
}

Semaphore::~Semaphore(){
    sem_destroy(&m_sem);
}

void Semaphore::wait(){
    if(sem_wait(&m_sem)){
        WYZ_LOG_ERROR(s_logger) << "wait fail";
        throw std::logic_error("wait error");
    }
}               // 实现信号量减1

void Semaphore::post(){
    if(sem_post(&m_sem)){
         WYZ_LOG_ERROR(s_logger) << "post fail";
        throw std::logic_error("post error");
    }
}              // 实现信号量加一    


}