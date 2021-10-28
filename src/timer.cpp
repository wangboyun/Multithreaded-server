/*
 * @Description: 定时器模块
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-22 10:12:52
 */

#include "timer.h"
#include "util.h"
#include <functional>
#include <memory>

namespace wyz {

Timer::Timer(uint64_t ms , std::function<void ()> cb , bool circular , TimerManager* manager)
    : m_ms(ms)
    , m_cb(cb)
    , m_circular(circular)
    , m_manager(manager){
    m_next = wyz::GetCurrentMS() + m_ms;
}

Timer::Timer(uint64_t next): m_next(next){
}

/* 取消当前的定时器 */
bool Timer::cancel(){
    TimerManager::RWMutexType::WriteLock wlock(m_manager->m_mutex);
    if(m_cb){
        m_cb = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}

/// 刷新设置定时器的执行时间 为当前的时间
bool Timer::refresh(){
    TimerManager::RWMutexType::WriteLock wlock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    } 
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    m_next = wyz::GetCurrentMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this());

    return true;
}

/**
* @brief 重置定时器时间
* @param[in] ms 定时器执行间隔时间(毫秒)
* @param[in] from_now 是否从当前时间开始计算
*/
bool Timer::reset(uint64_t ms, bool from_now){
    if(ms == m_ms && !from_now) {
        return true;
    }
    TimerManager::RWMutexType::WriteLock wlock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    } 
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    uint64_t start = 0;
    if(from_now){
        start = wyz::GetCurrentMS();
    }else {
        start = m_next - m_ms ;
    }
    m_ms = ms;
    m_next = start + m_ms;  
    m_manager->addTimer(shared_from_this(),wlock);

    return true;
}

/* 给 set 提供比较函数*/
bool Timer::Comparator::operator() (const Timer::ptr& lhs , const Timer::ptr& rhs )const{
    if(!lhs && !rhs )
        return false;
    if(!lhs)
        return false;
    if(!rhs)
        return true;
    if(lhs->m_next < rhs->m_next)
        return true;
    if(rhs->m_next < lhs->m_next)
        return false;
    return lhs.get() < rhs.get();
}


TimerManager::TimerManager(){
    m_previouseTime = wyz::GetCurrentMS();
}
TimerManager::~TimerManager(){}

Timer::ptr TimerManager::addTimer(uint64_t ms , std::function<void ()> cb , bool circular ){
    Timer::ptr timer(new Timer(ms , cb ,circular , this));
    RWMutexType::WriteLock wlock(m_mutex);
    addTimer(timer , wlock);
    return timer;
}

void TimerManager::addTimer(Timer::ptr timer , RWMutexType::WriteLock& lock){
    auto it = m_timers.insert(timer).first;
    bool at_front = (it == m_timers.begin()) && !m_titckled;
    if(at_front){
        m_titckled = true;
    }
    lock.unlock();
    if(at_front){
        onTimerInsertedAtFront();
    }
}

/* weak_ptr 不会添加 shared_ptr 的计数器的个数*/
static void OnTimer(std::weak_ptr<void> weak_cond , std::function<void()> cb){
    /* weak_ptr 与 shared_ptr 关联的智能指针存在则返回一个 shared_ptr 否则返回 nullptr */
    std::shared_ptr<void> temp = weak_cond.lock();
    if(temp){
        cb();
    }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms , std::function<void ()> cb ,std::weak_ptr<void> weak_cond , bool circular ){
    return addTimer(ms, std::bind(&OnTimer, weak_cond , cb) , circular);
}

/* 距离下一个定时器任务的时间间隔 */
uint64_t TimerManager::getNextTimer(){
    RWMutexType::ReadLock rlock(m_mutex);
    m_titckled = false;
    if(m_timers.empty()){
        return ~0ull;       /// 无定时器
    }
    const Timer::ptr& next = *m_timers.begin();
    uint64_t now_ms = wyz::GetCurrentMS();
    if(now_ms > next->m_next){
        return 0;
    }else {
        return next->m_next - now_ms;
    }
}

/* 超时定时器的 回调函数集合 */
void TimerManager::listExpiredCb(std::vector<std::function<void()> >& cbs){
    /// 当前的时间
    uint64_t now_ms = wyz::GetCurrentMS();
    std::vector<Timer::ptr> expired;        // 超时的计时器
    {
        RWMutexType::ReadLock rlock(m_mutex);
        if(m_timers.empty()){
            return;
        }
    }
    RWMutexType::WriteLock wlock(m_mutex);
    /// 最近的定时器 时间都小于当前时间
    bool rollover = detectClockRollover(now_ms);
    if( !rollover && (*m_timers.begin())->m_next > now_ms){
        return ;
    }
    /// 找到m_timers 中超时的定时器
    Timer::ptr now_timer(new Timer(now_ms));
    auto it = rollover? m_timers.end() :m_timers.lower_bound(now_timer);
    while(it != m_timers.end() && (*it)->m_next < now_ms){
        ++it;
    }
    expired.insert(expired.begin() , m_timers.begin() , it);

    /// 清除 m_timers 中超时的定时器
    m_timers.erase(m_timers.begin() , it);

    /// 存起超时定时器的回调函数
    cbs.resize(expired.size());
    for(auto timer : expired){
        cbs.emplace_back(timer->m_cb);
        /// 环形定时器
        if(timer->m_circular){
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer);
        }else {
            timer->m_cb = nullptr;
        }
    }

}

bool TimerManager::detectClockRollover(uint64_t now_ms){
    bool rollover = false;
    if(now_ms < m_previouseTime && now_ms < (m_previouseTime - 60 * 60 * 1000)){
        rollover = true;
    }
    m_previouseTime = now_ms;
    return rollover;
}

bool TimerManager::hasTimer(){
    RWMutexType::ReadLock rlock(m_mutex);
    return !m_timers.empty();
}

}
