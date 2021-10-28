/*
 * @Description:  IO 协程调度模块
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-20 11:21:35
 */

#ifndef __WYZ_IOMANAGE_H__
#define __WYZ_IOMANAGE_H__

#include "scheduler.h"
#include "timer.h"
#include <functional>

namespace wyz {

class IOManager : public Scheduler , public TimerManager{
public:
    using ptr = std::shared_ptr<IOManager>;
    using RWMutexType = RWMutex;

    /* 事件类型 */
    enum EventType{
        NONE  = 0x000,    // 无事件
        READ  = 0x001,    // 读事件 = EPOOLIN
        WRITE = 0x004,    // 写事件 = EPOOLOUT    
    };
private:
    /* 任务事件上下文类*/
    struct FdContext{
        using MutexTpye = Mutex;

        /*事件上下文类*/
        struct EventContext{
            Scheduler* scheduler = nullptr;
            Fiber::ptr fiber;
            std::function<void ()> cb;
        };
        /* 获得内部事件（读 / 写） */
        EventContext& getContext(EventType event);
        /// 重置 scheduler fiber cb
        void resetContext(EventContext& eventctx);
        /// 触发事件
        void triggerEvent(EventType event);
        /// 读事件上下文
        EventContext read;
        /// 写事件上下文
        EventContext write;
        /// 事件关联的句柄
        int fd = 0;
        /// 当前的事件
        EventType events = NONE;
        /// 事件的Mutex
        MutexType mutex;
    };

public:
    /**
     * @brief 构造函数
     * @param[in] threads 线程数量
     * @param[in] use_caller 是否将调用线程包含进去
     * @param[in] name 调度器的名称
     */
    IOManager(size_t threads = 1 , bool use_caller = true , const std::string& name = "");

    ~IOManager();

    /**
     * @brief: 添加事件
     * @param {int} fd socket 句柄
     * @param {Event} event 事件类型
     * @param {function<void ()>} cb 事件回调函数
     * @return  0 -- 成功  -1 -- 失败
     */    
    int addEvent(int fd , EventType event , std::function<void ()> cb = nullptr);

    /**
     * @brief:  删除事件
     * @param {int} fd socket句柄
     * @param {Event} event 事件类型
     * @attention 不会触发事件
     */    
    bool delEvent(int fd , EventType event);

    /**
     * @brief: 取消事件
     * @param {int} fd
     * @param {Event} event
     * @attention 如果事件存在则触发事件
     */    
    bool cancelEvent(int fd , EventType event);

    /**
     * @brief: 取消所有事件
     * @param {int} fd
     */    
    bool cancelAll(int fd);

    static IOManager* GetThis();

protected:
    void tickle() override;        // 通知调度器有任务来临
    bool stopping() override;     // 是否可以正常结束
    void idle() override;        // 协程无任务可调度时执行idle协程
    void onTimerInsertedAtFront() override;  // 定时器容器首有元素通知

    bool stopping(uint64_t& timeout);
    void contextResize(size_t size);
    
private:
    /// epoll 文件句柄
    int m_epfd = 0;
    /// pipe 文件句柄 [0] -- read [1]-- write
    int m_tickleFds[2]; 
    /// 当前等待执行的事件数量
    std::atomic<size_t> m_pendingEventCount = {0};
    /// IOManager的Mutex
    RWMutexType m_mutex;
    /// 任务事件上下文的容器
    std::vector<FdContext*> m_fdContexts;
};

    
}

#endif