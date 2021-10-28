/*
 * @Description: socket 句柄管理文件
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-25 17:00:03
 */

#ifndef __WYZ_FDMANAGER_H__
#define __WYZ_FDMANAGER_H__

#include "mutex.h"
#include "singleton.h"
#include <memory>
#include <pthread.h>
#include <vector>

namespace wyz {

class FdCtx : public std::enable_shared_from_this<FdCtx>{
public:
    using ptr = std::shared_ptr<FdCtx>;

    FdCtx(int fd);
    ~FdCtx();
    bool init();

    bool isInit()const       {return m_isInit;}
    bool isSocket()const     {return m_isSocket;}
    bool isClosed()const     {return m_isClosed;}

    void setUserNonblock(bool v)     {m_userNonblock = v;}
    bool getUserNonblock()const   {return m_userNonblock;}

    void setSysNonblock(bool v)      {m_sysNonblock = v;}
    bool getSysNonblock()const     {return m_sysNonblock;}

    /**
     * @brief: 设置超时时间
     * @param {int} type SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
     * @param {uint64_t} val 时间毫秒
     */    
    void setTimeout(int type , uint64_t val);

    /**
     * @brief: 获取超时时间
     * @param {int} type SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
     */    
    uint64_t getTimeout(int type);

private:
    /// 是否初始化
    bool m_isInit :1;

    /// 是否为socket
    bool m_isSocket :1;

    /// 是否为系统非阻塞
    bool m_sysNonblock :1;

    /// 是否为用户主动非阻塞
    bool m_userNonblock :1;

    /// 文件是否关闭
    bool m_isClosed :1;

    /// 文件句柄
    int m_fd;

    /// 读超时时间
    uint64_t m_recvTimeout;

    /// 写超时时间
    uint64_t m_sendTimeout;
};

class FdManager{
public:
    using ptr = std::shared_ptr<FdManager>;
    using RWMutexType  = RWMutex;
    FdManager();

    /**
     * @brief: 获取/创建文件句柄类FdCtx
     * @param {int} fd 文件句柄
     * @param {bool} auto_creat 是否自动创建
     * @return 对应文件句柄类FdCtx::ptr
     */    
    FdCtx::ptr get(int fd , bool auto_creat = false);

    /**
     * @brief: 删除文件句柄
     * @param {int} fd 文件句柄
     */    
    void del(int fd);

private:    
    RWMutexType m_mutex;
    std::vector<FdCtx::ptr> m_datas;
};

/// 文件管理类的单例模型
using FdMar = Singleton<FdManager>;

}

#endif