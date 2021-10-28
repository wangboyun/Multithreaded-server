/*
 * @Description: 
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-25 17:00:10
 */

#include "hook.h"
#include "fdmanager.h"
#include <fcntl.h>
#include <sys/stat.h>


namespace wyz {

// static wyz::Logger::ptr g_logger = WYZ_LOG_NAME("system");

FdCtx::FdCtx(int fd)
    : m_isInit(false)
    , m_isSocket(false)
    , m_sysNonblock(false)
    , m_userNonblock(false)
    , m_isClosed(false)
    , m_fd(fd)
    , m_recvTimeout(-1)
    , m_sendTimeout(-1){
    init();
}

FdCtx::~FdCtx(){}

bool FdCtx::init(){
    if(m_isInit){
        return true;
    }
    m_sendTimeout = -1;
    m_recvTimeout = -1;

    struct stat sta;
    if(fstat(m_fd, &sta) == -1){
       m_isInit = false;
       m_isSocket = false;
    }else {
        if(S_ISSOCK(sta.st_mode)){
            m_isSocket = true;
            m_isInit = true;
        }
    }

    if(m_isSocket){
        int fl = fcntl_f(m_fd , F_GETFL , 0);
        // 没有设置为阻塞状态
        if(!(fl & O_NONBLOCK)){
            fcntl_f(m_fd , F_SETFL , fl | O_NONBLOCK);
        }
        m_sysNonblock = true;
    }else {
        m_sysNonblock = false;
    }

    m_userNonblock = false;
    m_isClosed = false;
    return m_isInit;
}

void FdCtx::setTimeout(int type , uint64_t val){
    if(type == SO_RCVTIMEO){
        m_recvTimeout = val;
    }else {
        m_sendTimeout = val;
    }
}
   
uint64_t FdCtx::getTimeout(int type){
    if(type == SO_RCVTIMEO){
        return m_recvTimeout;
    }else{
        return m_sendTimeout;
    }
}

FdManager::FdManager(){
    m_datas.resize(64);
}

FdCtx::ptr FdManager::get(int fd , bool auto_creat ){
    if(fd < 0)
        return nullptr;
    RWMutexType::ReadLock rlock(m_mutex);
    if(static_cast<int>(m_datas.size()) <= fd){
        if(!auto_creat){
            return nullptr;
        }
    }else {
        if(m_datas[fd] || !auto_creat){
            return m_datas[fd];
        }
    }
    rlock.unlock();
    
    /// auto_creat == true 的情况
    RWMutexType::WriteLock wlock(m_mutex);
    FdCtx::ptr fd_ctx(new FdCtx(fd));
    if(static_cast<int>(m_datas.size()) > fd){
        m_datas[fd] = fd_ctx; 
    }else {
        m_datas.resize(fd * 1.5);
        m_datas[fd] = fd_ctx;
    }
    return m_datas[fd];
}
 
void FdManager::del(int fd){
    RWMutexType::ReadLock rlock(m_mutex);
    if(static_cast<int>(m_datas.size()) <= fd){
        return ;
    }
    rlock.unlock();
    
    RWMutexType::WriteLock wlock(m_mutex);
    m_datas[fd].reset();
}

}