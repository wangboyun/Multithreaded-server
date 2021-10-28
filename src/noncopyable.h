/*
 * @Description: 
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-09 21:09:20
 */

#ifndef __WYZ_NONCOPYABLE_H__
#define __WYZ_NONCOPYABLE_H__


namespace wyz {

class Noncopyable{
public:
    Noncopyable() = default;
    ~Noncopyable() = default;
    
    Noncopyable(Noncopyable&) = delete;
    Noncopyable(Noncopyable&&) = delete;
    Noncopyable& operator=(Noncopyable&) = delete;
};

}

#endif