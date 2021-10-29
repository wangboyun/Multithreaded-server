/*
 * @Description: IO协程模块
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-20 11:21:49
 */

#include "iomanager.h"
#include "fiber.h"
#include "log.h"
#include "macro.h"
#include <algorithm>
#include <cstdint>
#include <fcntl.h>
#include <functional>
#include <sys/epoll.h>
#include <unistd.h>
#include <error.h>
#include <string.h>

namespace wyz {

static wyz::Logger::ptr g_logger = WYZ_LOG_NAME("system");

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::EventType event){
    switch (event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            WYZ_ASSERT2(false , "eventtype error");
    }
}

void IOManager::FdContext::resetContext(IOManager::FdContext::EventContext& eventctx){
    eventctx.scheduler = nullptr;
    eventctx.fiber.reset();
    eventctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::EventType event){
    WYZ_ASSERT(events & event);
    events = static_cast<EventType>(events & ~event);
    EventContext& ctx = getContext(event);
    if(ctx.cb){
        ctx.scheduler->schedule(ctx.cb);
    }else {
        ctx.scheduler->schedule(ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return ;
}


IOManager::IOManager(size_t threads , bool use_caller , const std::string& name )
    : Scheduler(threads , use_caller , name){
    // 创建 epoll
    m_epfd = epoll_create(1);
    WYZ_ASSERT(m_epfd > 0);

    // 创建管道
    int rt = pipe(m_tickleFds);
    WYZ_ASSERT(!rt);

    struct epoll_event ev; 
    memset(&ev, 0, sizeof(ev));
    /* EPOLLET -- epoll工作在ET模式的时候，必须使用非阻塞套接口*/
    ev.events = EPOLLIN | EPOLLET;      // (EPOLLIN -- 读 EPOLLET -- 边缘触发事件通知)
    ev.data.fd = m_tickleFds[0];        // 管道读端

    // 设置m_tickleFds[0] 为非阻塞状态
    rt = fcntl(m_tickleFds[0], F_SETFL , O_NONBLOCK); 
    WYZ_ASSERT(!rt);

    // 将管道的读端加入epoll兴趣列表
    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &ev);
    WYZ_ASSERT(!rt);

    contextResize(32);
    Scheduler::start();

}

IOManager::~IOManager(){
    Scheduler::stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);
    for(size_t i = 0 ; i < m_fdContexts.size() ; ++i){
        if(m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
    
}

void IOManager::contextResize(size_t size){
    m_fdContexts.resize(size);
    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}  

/* 添加事件 */
int IOManager::addEvent(int fd , EventType event , std::function<void ()> cb){
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock rlock(m_mutex);
    if(m_fdContexts.size() > static_cast<size_t>(fd)){
        fd_ctx = m_fdContexts[fd];
        rlock.unlock();
    }else {
        rlock.unlock();
        RWMutexType::WriteLock wlock(m_mutex);
        contextResize(fd * 1.5 );
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::MutexTpye::Lock mlock(fd_ctx->mutex);
    /* 现在要添加的事件与任务事件上下文的容器内存在的事件一样 */
    if(fd_ctx->events & event){
        WYZ_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd 
            << "event=" <<  (EPOLL_EVENTS)event
            << "fd_ctx->event =" << (EPOLL_EVENTS)fd_ctx->events;
        WYZ_ASSERT(!(fd_ctx->events & event));
    }

    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    struct epoll_event epevent;
    memset(&epevent , 0 , sizeof(epevent));
    epevent.events = EPOLLET | fd_ctx->events | event;
    epevent.data.ptr = fd_ctx;
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        WYZ_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
            << (EPOLL_EVENTS)fd_ctx->events;
            return -1;
    }
    ++m_pendingEventCount;
    /* 总觉得这里有问题 */
    fd_ctx->events = static_cast<EventType>(fd_ctx->events | event) ;

    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    WYZ_ASSERT(!event_ctx.scheduler 
            && !event_ctx.fiber
            && !event_ctx.cb);
    event_ctx.scheduler = Scheduler::GetThis();

    if(cb){
        event_ctx.cb.swap(cb);
    }else {
        event_ctx.fiber = Fiber::GetThis();
        WYZ_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC , "state=" << event_ctx.fiber->getState());
    }
    
    return 0;
}

      
bool IOManager::delEvent(int fd , EventType event){
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock rlock(m_mutex);
    if(m_fdContexts.size() <= static_cast<size_t>(fd)){
        return false;
    }else {
        fd_ctx = m_fdContexts[fd];
        rlock.unlock();
    }

    FdContext::MutexTpye::Lock mlock(fd_ctx->mutex);
    // 任务事件池中的类型与 要删除的类型不同 , 则不删除
    if(!(fd_ctx->events & event)){
        return false;
    }

    EventType new_event = static_cast<EventType>(fd_ctx->events & ~event);
    int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    struct epoll_event epevent;
    memset(&epevent , 0 , sizeof(epevent));
    epevent.events = EPOLLET | new_event;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        WYZ_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
            << (EPOLL_EVENTS)fd_ctx->events;
            return -1;
    }
    --m_pendingEventCount;
    fd_ctx->events = new_event;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;

}

/* cancel 作用在 epoll关注的事件列表里删除，并触发掉此事件 */ 
bool IOManager::cancelEvent(int fd , EventType event){
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock rlock(m_mutex);
    if(m_fdContexts.size() <= static_cast<size_t>(fd)){
        return false;
    }else {
        fd_ctx = m_fdContexts[fd];
        rlock.unlock();
    }

    FdContext::MutexTpye::Lock mlock(fd_ctx->mutex);
    // 任务事件池中的类型与 要删除的类型不同 , 则不删除
    if(!(fd_ctx->events & event)){
        return false;
    }

    EventType new_event = static_cast<EventType>(fd_ctx->events & ~event);
    int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    struct epoll_event epevent;
    memset(&epevent , 0 , sizeof(epevent));
    epevent.events = EPOLLET | new_event;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        WYZ_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
            << (EPOLL_EVENTS)fd_ctx->events;
            return -1;
    }
    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    
    return true;
}
   
bool IOManager::cancelAll(int fd){
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock rlock(m_mutex);
    if(m_fdContexts.size() <= static_cast<size_t>(fd)){
        return false;
    }else {
        fd_ctx = m_fdContexts[fd];
        rlock.unlock();
    }

    FdContext::MutexTpye::Lock mlock(fd_ctx->mutex);
    // 任务事件池中的类型与 要删除的类型不同 , 则不删除
    if(!(fd_ctx->events)){
        return false;
    }

    int op =  EPOLL_CTL_DEL;
    struct epoll_event epevent;
    memset(&epevent , 0 , sizeof(epevent));
    epevent.events = 0 ;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        WYZ_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
            << (EPOLL_EVENTS)fd_ctx->events;
            return -1;
    }
    /* 只关注读写事件 */
    if(fd_ctx->events & IOManager::READ){
        fd_ctx->triggerEvent(IOManager::READ);
        --m_pendingEventCount;
    }
    if(fd_ctx->events & IOManager::WRITE){
        fd_ctx->triggerEvent(IOManager::WRITE);
        --m_pendingEventCount;
    }
    WYZ_ASSERT(fd_ctx->events == IOManager::NONE);
    return true;
}

IOManager* IOManager::GetThis(){
    // dynamic_cast  安全的向下转化 ， 基类  --> 派生类
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}


void IOManager::tickle(){
    if(hasIdelThreads()){
        int rt = write(m_tickleFds[1] , "T" , 1);
        WYZ_ASSERT(rt == 1);
    }
}        // 通知调度器有任务来临

bool IOManager::stopping(uint64_t& timeout){
    timeout = getNextTimer();
    return timeout == ~0ull        /// 没有定时器
        && m_pendingEventCount == 0
        && Scheduler::stopping() ;
}


bool IOManager::stopping(){
    uint64_t timerour = 0;
    return stopping(timerour);
}     // 是否可以正常结束

void IOManager::idle(){
    const uint64_t MAX_EVENTS = 256;
    epoll_event* events = new epoll_event[MAX_EVENTS];
    /* 智能指针初始化可以指定删除器 自动调用删除器来释放对象的内存。删除器也可以是一个lambda表达式*/
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){
        delete[] ptr;
    });
    while (true) {
        uint64_t timerout = 0;
        if(stopping(timerout)){
            WYZ_LOG_INFO(g_logger) << "name= "  << getName() << " idle exit";
            break; 
        }
        int rt = 0;
        do {
            static const int MAX_TIMEOUT = 3000;
            if(timerout != ~0ull){
                timerout = std::min(static_cast<int>(timerout),MAX_TIMEOUT);
            }else{
                timerout = MAX_TIMEOUT;
            }
            rt = epoll_wait(m_epfd, events, MAX_EVENTS, static_cast<int>(timerout));
            if(rt < 0 && errno == EINTR){
                continue;
            }else {
                break;
            }
        }while (true);

        std::vector<std::function<void ()>> cbs;
        listExpiredCb(cbs);
        // 超时的定时器加入到任务池队列里
        if(!cbs.empty()){
            // WYZ_LOG_DEBUG(g_logger) << "on timer cbs.size=" << cbs.size();
            schedule(cbs.begin() , cbs.end());
            cbs.clear();
        }
        for(int i = 0 ; i < rt ; ++i){
            epoll_event& event = events[i];
            if(event.data.fd == m_tickleFds[0]){
                uint8_t dummy[256];
                while (read(m_tickleFds[0] , dummy , sizeof(dummy)) > 0);
                continue;
            }  
            FdContext* fd_ctx = static_cast<FdContext*>(event.data.ptr);
            FdContext::MutexTpye::Lock lock(fd_ctx->mutex);
            if(event.events & (EPOLLERR | EPOLLHUP)){
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
            }
            int real_events = IOManager::NONE;
            if(event.events & EPOLLIN){
                real_events |= IOManager::READ;
            }
            if(event.events & EPOLLOUT){
                real_events |= IOManager::WRITE;
            }
            if((fd_ctx->events & real_events) == IOManager::NONE){
                continue;
            }

            int left_events = (fd_ctx->events & ~real_events);
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

            event.events = EPOLLET | left_events;

            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(rt2){
                WYZ_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events << "):"
                    << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }
            if(real_events & IOManager::READ){
                fd_ctx->triggerEvent(IOManager::READ);
                --m_pendingEventCount;
            }
            if(real_events & IOManager::WRITE){
                fd_ctx->triggerEvent(IOManager::WRITE);
                --m_pendingEventCount;
            }
        }
        /* 出让调度器 */
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();

        raw_ptr->swapOut();

    }

}        // 协程无任务可调度时执行idle协程陷入epoll_waite 等待

void IOManager::onTimerInsertedAtFront(){
    tickle();
}


}