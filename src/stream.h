/**
 * @file stream.h
 * @brief 封装流接口
 * @author wyz (501826086@qq.com)
 * @version 1.0
 * @date 2021-11-18
 * 
 * @copyright Copyright (c) 2021  wyz
 * 
 */

#ifndef __WYZ_STREAM_H__
#define __WYZ_STREAM_H__

#include <cstddef>
#include <memory>
#include "bytearray.h"

namespace wyz {

class Stream{
public:
    using ptr = std::shared_ptr<Stream>;
    virtual ~Stream() {}
    /**
     * @brief  读操作
     * @param  buff             My Param doc
     * @param  len              My Param doc
     * @return int 
     */
    virtual int read(void * buff , size_t length) = 0;
    virtual int read(ByteArray::ptr ba , size_t length) = 0;
    virtual int readFixSize(void * buff , size_t length);
    virtual int readFixSize(ByteArray::ptr ba , size_t length) ;

    /**
     * @brief 写操作
     * @param  buff             My Param doc
     * @param  len              My Param doc
     * @return int 
     */
    virtual int write(const void * buff , size_t length) = 0;
    virtual int write(ByteArray::ptr ba , size_t length) = 0;
    virtual int writeFixSize(const void * buff , size_t length);
    virtual int writeFixSize(ByteArray::ptr ba , size_t length);

    virtual int close() = 0;
};
}

#endif