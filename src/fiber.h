/*
 * @Description:  协程封装
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-12 15:33:12
 */

#ifndef __WYZ_FIBER_H__
#define __WYZ_FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>


namespace wyz {

class Scheduler;

class Fiber : public std::enable_shared_from_this<Fiber>{
friend class Scheduler;
public:
    using ptr = std::shared_ptr<Fiber>;
    enum State{
        INIT,       //初始态
        HOLD,       //暂停态
        EXEC,       // 执行态
        TERM,       // 结束态
        READY,      // ready态
        EXCEPT      // 异常态
    };

private:
    /**
     * @brief 无参构造函数
     * @attention 每个线程第一个协程的构造
     */
    Fiber();

public: 
    /**
     * @description: 
     * @param {function<void ()>} cb 协程运行函数
     * @param {size_t} stacksize    协程栈大小
     */    
    Fiber(std::function<void ()> cb , size_t stacksize = 0 , bool use_caller = false);
    ~Fiber();

    /**
     * @description: 重置协程运行函数
     * @param {function<void ()>} cb 
     */    
    void reset(std::function<void ()> cb , bool use_caller = false);

    /**
     * @description: 当前协程切换为运行状态
     */    
    void swapIn();

    /**
     * @description: 将当前协程切换至后台 
     */    
    void swapOut();

    /**
     * @brief 将当前线程切换到执行状态
     * @pre 执行的为当前线程的主协程
     */
    void call();

    /**
     * @brief 将当前线程切换到后台
     * @pre 执行的为该协程
     * @post 返回到线程的主协程
     */
    void resume();

    inline uint64_t getId() const {return m_id;};
    inline State getState()const {return m_state;};

public:
    /**
     * @description: 设置当前协程的运行协程
     * @param {Fiber*} f 运行协程
     */    
    static void SetThis(Fiber* f);

    /**
     * @brief: 获取当前的运行协程
     */     
    static Fiber::ptr GetThis();

    /**
     * @brief: 将当前协程切换至后台，并设置state = Ready态
     */    
    static void YieldToReady();
    static void CallerYieldToReady();
    /**
     * @brief: 将当前协程切换至后台，并设置state = Hold态
     */   
    static void YieldToHold();
    static void CallerYieldToHold();
    /**
     * @brief: 返回协程的总数量
     */   
    static uint64_t TotalFibers();

    /**
     * @brief: 返回当前协程调度器
     */    
    static void MainFunc();

    /**
     * @brief: 返回当前协程调度器的调度协程
     */    
    static void CallerMainFunc();

    /**
     * @brief 获取当前协程的id
     */
    static uint64_t GetFiberId();

private:
    uint64_t m_id = 0 ;             //  协程id
    uint32_t m_stacksize = 0;       //  协程运行栈大小
    State m_state = INIT;           //  协程状态
    ucontext_t m_ctx;               //  协程运行现场上下文
    void* m_stack = nullptr;        //  协程运行栈指针
    std::function<void ()> m_cb;    //  协程执行函数
};

}

#endif