/*
 * @Description: 
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-11 16:33:41
 */

#ifndef __WYZ_MUTEX_H__
#define __WYZ_MUTEX_H__

#include <pthread.h>
#include <thread>
#include <semaphore.h>
#include "noncopyable.h"

namespace wyz {

/* 封装信号量类 */
class Semaphore : Noncopyable{
public:

    Semaphore(const unsigned int val = 0);
    ~Semaphore();

    void wait();                // 实现信号量减1
    void post();               // 实现信号量加一    

private:
    sem_t m_sem;
};

/* 封装互斥量mutex */
/* 先封装一个万能的互斥量类 */
template<class T>
class ScopedLockImpl{
public:
    ScopedLockImpl(T& mutex)
    : m_mutex(mutex){
        m_mutex.lock();
        m_locked = true;
    }

    ~ScopedLockImpl(){
        if(m_locked){
            m_mutex.unlock();
        }
    }

    void lock(){
        if(!m_locked){
            m_mutex.lock();
            m_locked = true;
        }
    }

    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked = false;
};

/* 封装一个局部 读锁*/
template<class T>
class ReadScopedLockImpl{
public:
    ReadScopedLockImpl(T& mutex)
    : m_mutex(mutex){
        m_mutex.rdlock();
        m_locked = true;
    }

    ~ReadScopedLockImpl(){
        if(m_locked){
            m_mutex.unlock();
        }
    }

    void lock(){
        if(!m_locked){
            m_mutex.lock();
            m_locked = true;
        }
    }

    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked = false;
};

/* 封装一个局部写锁*/
template<class T>
class WriteScopedLockImpl{
public:
    WriteScopedLockImpl(T& mutex)
    : m_mutex(mutex){
        m_mutex.wrlock();
        m_locked = true;
    }

    ~WriteScopedLockImpl(){
        if(m_locked){
            m_mutex.unlock();
        }
    }

    void lock(){
        if(!m_locked){
            m_mutex.lock();
            m_locked = true;
        }
    }

    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked = false;
};

/* pthred 互斥量*/
class Mutex : Noncopyable{
public: 
    using Lock = ScopedLockImpl<Mutex>;

    Mutex(){
        pthread_mutex_init(&m_mutex , nullptr);
    }

    ~Mutex(){
        pthread_mutex_destroy(&m_mutex);
    }

    void lock(){
        pthread_mutex_lock(&m_mutex);
    }

    void unlock(){
        pthread_mutex_unlock(&m_mutex);
    }

private:
    pthread_mutex_t m_mutex;
};

/* 空互斥量锁便于调试 */
class NULLMutex : Noncopyable{
public:
using Lock = ScopedLockImpl< NULLMutex >;

    NULLMutex(){}

    ~NULLMutex(){}

    void lock(){}

    void unlock(){}
private:

};


/* 读写锁 */
class RWMutex : Noncopyable{
public:
    using ReadLock = ReadScopedLockImpl< RWMutex >;
    using WriteLock = WriteScopedLockImpl< RWMutex >;

    RWMutex(){
        pthread_rwlock_init(&m_rwlock, nullptr);
    }

    ~RWMutex(){
        pthread_rwlock_destroy(&m_rwlock);
    }

    void rdlock(){
        pthread_rwlock_rdlock(&m_rwlock);
    }

    void wrlock(){
        pthread_rwlock_wrlock(&m_rwlock);
    }

    void unlock(){
        pthread_rwlock_unlock(&m_rwlock);
    }

private:
    pthread_rwlock_t m_rwlock;
};

/* 空的读写锁 */
class NULLRWMutex{
public:
    using ReadLock = ReadScopedLockImpl< NULLRWMutex >;
    using WriteLock = WriteScopedLockImpl< NULLRWMutex >;

    NULLRWMutex(){}
       
    ~NULLRWMutex(){}
       
    void rdlock(){}

    void wrlock(){}

    void unlock(){}
private:

};

/*封装自旋锁*/
class SpinLock : Noncopyable{
public:
    using Lock = ScopedLockImpl<SpinLock>;

    SpinLock(){
        pthread_spin_init(&m_mutex,0);
    }

    ~SpinLock(){
        pthread_spin_destroy(&m_mutex);
    }

    void lock(){
        pthread_spin_lock(&m_mutex);
    }

    void unlock(){
        pthread_spin_unlock(&m_mutex);
    }

private:
    pthread_spinlock_t m_mutex;
};


}
#endif