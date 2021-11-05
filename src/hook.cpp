/*
 * @Description: 
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-25 10:38:50
 */

#include "hook.h"
#include "fiber.h"
#include "scheduler.h"
#include "iomanager.h"
#include "fdmanager.h"
#include "log.h"
#include "config.h"

#include <dlfcn.h>
#include <memory>
#include <sys/socket.h>
#include <stdarg.h>

namespace wyz {

static wyz::Logger::ptr g_logger = WYZ_LOG_NAME("system");
static wyz::ConfigVar<int>::ptr g_tcp_connect_timeout = wyz::Config::Lookup("tcp.connect.timeout", 5000,"tcp connect timeout");

static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX)\
    XX(sleep)\
    XX(usleep)\
    XX(nanosleep)\
    XX(socket)\
    XX(connect)\
    XX(accept)\
    XX(read)\
    XX(readv)\
    XX(recv)\
    XX(recvfrom)\
    XX(recvmsg)\
    XX(write)\
    XX(writev)\
    XX(send)\
    XX(sendto)\
    XX(sendmsg)\
    XX(close)\
    XX(fcntl)\
    XX(ioctl)\
    XX(getsockopt)\
    XX(setsockopt)\

void hook_init(){
    static bool is_inited = false;
    if(is_inited){
        return;
    }
/// dlsym 找到原始的系统调用原型
#define XX(name) name ##_f = (name ##_func)dlsym(RTLD_NEXT , #name);
    HOOK_FUN(XX);
#undef XX

}

static uint64_t s_connect_timeout = -1;
struct _HookIniter{
    _HookIniter(){
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();
        g_tcp_connect_timeout->addListener([](const int& old_val , const int& new_val){
            WYZ_LOG_INFO(g_logger) << "tcp connect timeout changed from " << old_val << " to " << new_val;
            s_connect_timeout = new_val;
        });
        
    }
};

/// 静态全局变量在 main 函数之前执行其构造函数
static _HookIniter s_hook_initer;

/// 检测是否 hook 了
bool isHookEnable(){
    return t_hook_enable;
}

/// 设置是否 hook 
void setHookEnable(bool flag){
    t_hook_enable = flag;
}

struct timer_info{
    int cancelled = 0;
};

/* 通用的 io 处理函数 */
template<typename OriginFun , typename... Args>
static ssize_t do_io(int fd , OriginFun fun , const char* hook_fun_name , uint32_t event , int timeout_so , Args&&... args){
    /// 没被 hook
    if(!wyz::t_hook_enable){
        return fun(fd , std::forward<Args>(args)...);
    }

    
    wyz::FdCtx::ptr ctx = wyz::FdMar::GetInstance()->get(fd);
    if(!ctx){
        return fun(fd , std::forward<Args>(args)...);
    }
    /// 文件关闭了
    if(ctx->isClosed()){
        errno = EBADF;
        return -1;
    }
    /// 文件不是socket 或者用户主动设置为非阻塞状态
    if(!ctx->isSocket() || ctx->getUserNonblock()){
        return fun(fd , std::forward<Args>(args)...);
    }
    /// 获得超时时间
    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
    ssize_t n = fun(fd , std::forward<Args>(args)...);
    while(n == -1 && errno == EINTR){
        n = fun(fd , std::forward<Args>(args)...);
    }
    if(n == -1 && errno == EAGAIN){
        /// fun 原函数是阻塞类型的io
        wyz::IOManager* iom = wyz::IOManager::GetThis();
        wyz::Timer::ptr timer;
        std::weak_ptr<timer_info> winfo(tinfo);

        /// 有超时时间
        if(to != static_cast<uint64_t>(-1)){
            timer = iom->addConditionTimer(to, [winfo , fd , iom , event](){
                auto it = winfo.lock();
                if(!it || it->cancelled){
                    return ;
                }
                it->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, static_cast<wyz::IOManager::EventType>(event));
            },winfo);
        }

        /// 没有设置超时时间的时候 ，将这个事件添加到 epoll 中去，然后出让调度器
        int rt = iom->addEvent(fd, static_cast<wyz::IOManager::EventType>(event));
        if(rt){
            WYZ_LOG_ERROR(g_logger) << hook_fun_name << " addEvent(" << fd << ", "<< event << ")";
            if(timer){
                timer->cancel();
            }
            return -1;
        }else {
            /// 这里就是当 io 操作是阻塞状态的时候， 出让调度器，等 epoll 唤醒这给
            wyz::Fiber::CallerYieldToHold();
        
            if(timer){
                timer->cancel();
            }
            if(tinfo->cancelled){
                errno = tinfo->cancelled;
                return -1;
            }
            goto retry;
        }

    }
    return n;

}


extern "C"{
#define XX(name) name ##_func name ##_f = nullptr;
    HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds){
    if(!wyz::t_hook_enable){
        return sleep_f(seconds);
    }
    wyz::Fiber::ptr fiber = wyz::Fiber::GetThis();
    wyz::IOManager* iom = wyz::IOManager::GetThis();
    iom->addTimer(seconds * 1000, std::bind((void(wyz::Scheduler::*)
            (wyz::Fiber::ptr, int thread))&wyz::IOManager::schedule
            ,iom, fiber, -1));
    wyz::Fiber::CallerYieldToHold();
    return 0;
}

int usleep(useconds_t usec){
    if(!wyz::t_hook_enable){
        return usleep_f(usec);
    }
    wyz::Fiber::ptr fiber = wyz::Fiber::GetThis();
    wyz::IOManager* iom = wyz::IOManager::GetThis();
    iom->addTimer(usec / 1000, std::bind((void(wyz::Scheduler::*)
            (wyz::Fiber::ptr, int thread))&wyz::IOManager::schedule
            ,iom, fiber, -1));
    wyz::Fiber::CallerYieldToHold();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem){
    if(!wyz::t_hook_enable){
        return nanosleep_f(req,rem);
    }
    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000 ;
    wyz::Fiber::ptr fiber = wyz::Fiber::GetThis();
    wyz::IOManager* iom = wyz::IOManager::GetThis();
    iom->addTimer(timeout_ms , std::bind((void(wyz::Scheduler::*)
            (wyz::Fiber::ptr, int thread))&wyz::IOManager::schedule
            ,iom, fiber, -1));
    wyz::Fiber::CallerYieldToHold();
    return 0;
}

int socket(int domain, int type, int protocol){
    if(!wyz::t_hook_enable){
        return socket_f(domain , type , protocol);
    }
    int fd = socket_f(domain , type , protocol);
    if(fd < 0){
        return fd;
    }
    wyz::FdMar::GetInstance()->get(fd , true);
    return fd;
}

int connect_with_timeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen , uint64_t timeout_ms){
    if(!wyz::t_hook_enable){
        return connect_f(sockfd, addr , addrlen);
    }

    // WYZ_LOG_DEBUG(g_logger) << "nonblock";
    wyz::FdCtx::ptr ctx = wyz::FdMar::GetInstance()->get(sockfd);
    if(!ctx || ctx->isClosed()){
        errno = EBADF;
        return -1;
    }
    if(!ctx->isSocket() || ctx->getUserNonblock()){
        return connect_f(sockfd, addr , addrlen);
    }

    int n  = connect_f(sockfd, addr , addrlen);
    if(n == 0){
        return 0;
    }else if( n != -1 || errno != EINPROGRESS){
        return n;
    }
    
    wyz::IOManager* iom = wyz::IOManager::GetThis();
    wyz::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);

    /// 有超时时间
    if(timeout_ms != static_cast<uint64_t>(-1)){
        timer = iom->addConditionTimer(timeout_ms, [winfo , iom ,sockfd](){
            auto it = winfo.lock();
            if(!it || it->cancelled){
                return ;
            }
            it->cancelled = ETIMEDOUT;
            iom->cancelEvent(sockfd,wyz::IOManager::WRITE);
        }, winfo);
    }
    int rt = iom->addEvent(sockfd, wyz::IOManager::WRITE);
    if(rt){
        WYZ_LOG_ERROR(g_logger) << "connect addEvent(" << sockfd << ", WRITE) error";
        if(timer){
            timer->cancel();
        }
    }else {
        wyz::Fiber::CallerYieldToHold();
        if(timer){
            timer->cancel();
        }
        if(tinfo->cancelled){
            errno = tinfo->cancelled;
            return -1;
        }
    }

    int error = 0;
    socklen_t len = sizeof(int);
    if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) == -1){
        return -1;
    }
    if(!error){
        return 0;
    }else {
        errno = error;
        return -1;
    }

}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
    return connect_with_timeout(sockfd, addr, addrlen, wyz::s_connect_timeout);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen){
    int fd = do_io(sockfd, accept_f, "accept", wyz::IOManager::READ, SO_RCVTIMEO, addr , addrlen);
    if(fd >= 0){
        wyz::FdMar::GetInstance()->get(fd ,true);
    }
    return fd;
}

ssize_t read(int fd, void *buf, size_t count){
    return do_io(fd, read_f, "read", wyz::IOManager::READ, SO_RCVTIMEO, buf , count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt){
    return do_io(fd, readv_f, "readv", wyz::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags){
    return do_io(sockfd, recv_f, "recv", wyz::IOManager::READ, SO_RCVTIMEO, buf ,len ,flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen){
    return do_io(sockfd, recvfrom_f, "recvfrom", wyz::IOManager::READ, SO_RCVTIMEO, buf , len , flags ,src_addr , addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags){
    return do_io(sockfd, recvmsg_f, "recvmsg", wyz::IOManager::READ, SO_RCVTIMEO, msg ,flags);
}

ssize_t write(int fd, const void *buf, size_t count){
    return do_io(fd, write_f, "erite", wyz::IOManager::WRITE, SO_SNDTIMEO, buf , count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt){
    return do_io(fd, writev_f, "writev",  wyz::IOManager::WRITE, SO_SNDTIMEO, iov , iovcnt);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags){
    return do_io(sockfd, send_f, "send", wyz::IOManager::WRITE, SO_SNDTIMEO, buf ,len , flags);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen){
    return do_io(sockfd, sendto_f, "sendto", wyz::IOManager::WRITE, SO_SNDTIMEO, buf , len ,flags ,dest_addr ,addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags){
    return do_io(sockfd, sendmsg_f, "sendmsg", wyz::IOManager::WRITE, SO_SNDTIMEO, msg ,flags);
}

int close(int fd){
    if(!wyz::t_hook_enable){
        return close_f(fd);
    }
    wyz::FdCtx::ptr ctx = wyz::FdMar::GetInstance()->get(fd);
    if(ctx){
        wyz::IOManager* iom = wyz::IOManager::GetThis();
        iom->cancelAll(fd);
        wyz::FdMar::GetInstance()->del(fd);
    }
    
    return close_f(fd);
}

int fcntl(int fd, int cmd, ... /* arg */ ){
    va_list va;
    va_start(va, cmd);

    switch (cmd) {
        case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                wyz::FdCtx::ptr ctx = wyz::FdMar::GetInstance()->get(fd);
                if(!ctx || ctx->isClosed() || !ctx->isSocket()){
                    return fcntl_f(fd , cmd , arg);
                }
                ctx->setUserNonblock(arg & O_NONBLOCK);
                if(ctx->getSysNonblock()){
                    arg |= O_NONBLOCK;
                }else {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd , cmd , arg);
            }
            break;
        case F_GETFL:
            {
                va_end(va);
                int arg = fcntl_f(fd , cmd);
                wyz::FdCtx::ptr ctx = wyz::FdMar::GetInstance()->get(fd);
                if(!ctx || ctx->isClosed() || !ctx->isSocket()){
                    return arg;
                }
                if(ctx->getUserNonblock()){
                    return arg | O_NONBLOCK;
                }else {
                    return arg & ~O_NONBLOCK;
                }
            
            }   
            break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
        case F_SETPIPE_SZ:
        case F_ADD_SEALS:
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd , cmd , arg);
            }
            break;
        case  F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
        case F_GETPIPE_SZ:
        case F_GET_SEALS:
            {
                return fcntl_f(fd , cmd );
            }
            break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
        case F_OFD_SETLK:
        case F_OFD_SETLKW:
        case F_OFD_GETLK:
            {
                struct flock* arg = va_arg(va, struct flock*);
                va_end(va);
                return fcntl_f(fd , cmd , arg);
            }
            break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_ex* arg = va_arg(va, struct f_owner_ex*);
                va_end(va);
                return fcntl_f(fd , cmd , arg);
            }
            break;
        default:
            va_end(va);
            return fcntl_f(fd , cmd );
    }
    return 0;
}

int ioctl(int fd, unsigned long request, ...){

    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);
    
    if(request == FIONBIO){
        bool user_nonblock = !!*static_cast<int *>(arg);
        wyz::FdCtx::ptr ctx = wyz::FdMar::GetInstance()->get(fd);
        if(!ctx || ctx->isClosed() || !ctx->isSocket() ){
            ioctl_f(fd , request , arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(fd , request , arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen){
    return getsockopt_f(sockfd , level , optname ,optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen){
    if(!wyz::t_hook_enable){
        return setsockopt_f(sockfd , level ,optname , optval , optlen);
    }
    if(level == SOL_SOCKET){
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO){
            wyz::FdCtx::ptr ctx = wyz::FdMar::GetInstance()->get(sockfd);
            if(ctx){
                const timeval* v = (const timeval*) optval;
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }
    return  setsockopt_f(sockfd , level ,optname , optval , optlen);
}
    
}

}