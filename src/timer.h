/*
 * @Description: 定时器模块
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-22 10:12:43
 */

#ifndef __WYZ_TIMER_H__
#define __WYZ_TIMER_H__

#include "mutex.h"
#include <functional>
#include <memory>
#include <set>
#include <vector>

namespace wyz {

/// 定时器管理类
class TimerManager;

/// 定时器类
class Timer : public std::enable_shared_from_this<Timer>{
friend class TimerManager;
public:
    using ptr = std::shared_ptr<Timer>;
    /**
     * @brief 取消定时器
     */
    bool cancel();

    /**
     * @brief 刷新设置定时器的执行时间
     */
    bool refresh();

    /**
     * @brief 重置定时器时间
     * @param[in] ms 定时器执行间隔时间(毫秒)
     * @param[in] from_now 是否从当前时间开始计算
     */
    bool reset(uint64_t ms, bool from_now);
    
private:
    /**
     * @brief: 定时器构造函数
     * @param {uint64_t} ms 事件间隔    
     * @param {function<void ()>} cb 回调函数
     * @param {bool} circular   是否是循环定时器
     * @param {TimerManager*} manager 定时器管理
     */
    Timer(uint64_t ms , std::function<void ()> cb , bool circular , TimerManager* manager);

    Timer(uint64_t next);

private:
    uint64_t m_ms = 0;              // 执行周期
    std::function<void ()> m_cb;    // 执行的回调函数
    bool m_circular = false;
    TimerManager* m_manager = nullptr;
    uint64_t m_next = 0 ;           // 精确的执行时间

private:
    /**
     * @brief 定时器比较仿函数  ( 学习一下这样的操作 )
     */
    struct Comparator{
        bool operator() (const Timer::ptr& lhs , const Timer::ptr& rhs )const;
    };
};

class TimerManager{
friend class Timer;
public:
    using RWMutexType  =  RWMutex;
    TimerManager();
    virtual ~TimerManager();

    Timer::ptr addTimer(uint64_t ms , std::function<void ()> cb , bool circular = false);

    Timer::ptr addConditionTimer(uint64_t ms , std::function<void ()> cb ,std::weak_ptr<void> weak_cond , bool circular = false);

    /**
     * @brief 到最近一个定时器执行的时间间隔(毫秒)
     */
    uint64_t getNextTimer();

    /**
     * @brief 获取需要执行的定时器的回调函数列表
     * @param[out] cbs 回调函数数组
     */
    void listExpiredCb(std::vector<std::function<void()>>& cbs);

    /* 有没有定时器 */
    bool hasTimer();

protected:
    /// 定时器在定时器容器的最前面
    virtual void onTimerInsertedAtFront() = 0;

    void addTimer(Timer::ptr timer , RWMutexType::WriteLock& lock);

private:    
    /**
     * @brief 检测服务器时间是否被调后了
     */
    bool detectClockRollover(uint64_t now_ms);

private:
    /// 互斥量
    RWMutexType m_mutex;
    /// 定时器集合
    std::set<Timer::ptr , Timer::Comparator> m_timers;
    /// 触发onTimerInsertedAtFront
    bool m_titckled = false;
    /// 上次执行的时间
    uint64_t m_previouseTime = 0;
};

}


#endif