/*
 * @Description: 协程调度模块
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-13 19:56:23
 */

#ifndef __WYZ_SCHEDULER_H__
#define __WYZ_SCHEDULER_H__


#include <atomic>
#include <functional>
#include <list>
#include <memory>
#include <sched.h>
#include <vector>
#include "mutex.h"
#include "thread.h"
#include "fiber.h"

namespace wyz {

/**
 * @brief 协程调度器
 * @details 封装的是N-M的协程调度器
 *          内部有一个线程池,支持协程在线程池里面切换
 */
class Scheduler{
public:
    using ptr = std::shared_ptr<Scheduler>;
    using MutexType = Mutex;
    /**
     * @brief: 
     * @param {size_t} threads   线程数量
     * @param {bool} use_caller 是否使用当前调用线程
     * @param {string} name     协程调用器名称
     */    
    Scheduler(size_t threads = 1 , bool use_caller = true , const std::string& name = "");

    virtual ~Scheduler();

    inline const std::string& getName()const {return m_name;};

    static Scheduler* GetThis();                    // 获取当前的协程调度器
    static Fiber* GetMainFiber();               // 返回当前协程调度器的调度协程

    void start();

    void stop();

    /* 一个一个任务添加 */
    template<typename FiberOrCb>
    void schedule(FiberOrCb fc , int thread = -1){
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc , thread);
        }
        if(need_tickle){
            tickle();
        }
    }

    /* 批量添加任务*/
    template<typename  InputIterator>
    void schedule(InputIterator begin , InputIterator end){
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while(begin != end){
                need_tickle = scheduleNoLock(*begin, -1) || need_tickle;
                ++ begin;
            }
        }
        if(need_tickle){
            tickle();
        }
    }

protected:
    virtual void tickle();          // 通知调度器有任务来临
    void setThis();         // 设置当前的协程调度器
    void run();             // 协程调度函数
    virtual bool stopping();        // 是否可以正常结束
    virtual void idle();            // 协程无任务可调度时执行idle协程
    inline bool hasIdelThreads() {return m_idleThreadCount > 0;}
private:
    template<typename FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc , int threadid){
        bool need_tickle = m_fibers.empty();
        Task ft(fc , threadid);
        if(ft.fiber || ft.cb){
            m_fibers.emplace_back(ft);
        }
        return need_tickle;
    }

private:
    /* 需要被调度的协程或者协程执行函数类型 */
    struct Task{
        Fiber::ptr fiber;           // 协程
        std::function<void ()> cb;  // 协程执行函数
        int threadId;               // 线程id

        Task() : threadId(-1){}
        Task(Fiber::ptr f , int thr)
        : fiber(std::move(f)) // 使用移动构造之后 f指向空，shared计数器不会+1 
        , threadId(thr){}

        
        Task(std::function<void ()> f,int thr)
        :cb(std::move(f)) ,threadId(thr){
        }

        
        void reset(){
            fiber = nullptr;
            cb = nullptr;
            threadId = -1;
        }

    };

private:
    // 互斥量
    MutexType m_mutex;
    // 线程池
    std::vector<Thread::ptr> m_threads;
    // 待执行的协程队列
    std::list<Task> m_fibers;
    /// use_caller为true时有效, 调度协程
    Fiber::ptr m_rootFiber;
    // 协程器名称
    std::string m_name;

protected:
    std::vector<int> m_threadIds;       // 协程下的线程id数组
    size_t m_threadCount = 0;           // 线程数量
    // 工作的线程数量
    std::atomic<size_t> m_activeThreadCount = {0};
    // 空闲的线程数量
    std::atomic<size_t> m_idleThreadCount = {0};
    bool m_stopping = true;             // 是否正在停止
    bool m_autostop = false;            // 是否能自动停止
    pid_t m_rootThreadId = 0 ;            // 主线程id(use_caller)
};



}


#endif