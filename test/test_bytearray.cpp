/**
 * @file test_bytearray.cpp
 * @brief  字节序与反字节序测试
 * @author wyz (501826086@qq.com)
 * @version 1.0
 * @date 2021-11-10
 * 
 * @copyright Copyright (c) 2021  wyz
 * 
 */


#include "../src/log.h"
#include "../src/bytearray.h"
#include "../src/macro.h"


wyz::Logger::ptr g_logger = WYZ_LOG_ROOT();

void test_bytearray(){
#define XX(type , len , write_fun, read_fun , base_size) {\
    wyz::ByteArray::ptr ba(new wyz::ByteArray(base_size));\
    std::vector<type> vec;\
    for(int i = 0 ; i < len ; ++i){\
        vec.emplace_back(rand());\
    }\
    for(auto i : vec){\
        ba->write_fun(i);\
    }\
    ba->setPosition(0);\
    for(size_t i = 0 ; i < vec.size(); ++i){\
        type v = ba->read_fun();\
        WYZ_ASSERT(v == vec[i]);\
    }\
    WYZ_ASSERT(ba->getReadSize() == 0); \
    WYZ_LOG_INFO(g_logger) << #write_fun "/" #read_fun \
                    " (" #type " ) len=" << len \
                    << " base_len=" << base_size \
                    << " size=" << ba->getSize(); \
}
    XX(int8_t,  100, writeFint8, readFint8, 1);
    XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(int16_t,  100, writeFint16,  readFint16, 1);
    XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(int32_t,  100, writeFint32,  readFint32, 1);
    XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    XX(int64_t,  100, writeFint64,  readFint64, 1);
    XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t,  100, writeInt32,  readInt32, 1);
    XX(uint32_t, 100, writeUint32, readUint32, 1);
    XX(int64_t,  100, writeInt64,  readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);
#undef XX

#define XX(type, len, write_fun, read_fun, base_len) {\
    std::vector<type> vec; \
    for(int i = 0; i < len; ++i) { \
        vec.emplace_back(rand()); \
    } \
    wyz::ByteArray::ptr ba(new wyz::ByteArray(base_len)); \
    for(auto& i : vec) { \
        ba->write_fun(i); \
    } \
    ba->setPosition(0); \
    for(size_t i = 0; i < vec.size(); ++i) { \
        type v = ba->read_fun(); \
        WYZ_ASSERT(v == vec[i]); \
    } \
    WYZ_ASSERT(ba->getReadSize() == 0); \
    WYZ_LOG_INFO(g_logger) << #write_fun "/" #read_fun \
                    " (" #type " ) len=" << len \
                    << " base_len=" << base_len \
                    << " size=" << ba->getSize(); \
    ba->setPosition(0); \
    WYZ_ASSERT(ba->writeToFile("/home/wyz/workspace/wyz/bytearray/" #type "_" #len "-" #read_fun ".dat")); \
   wyz::ByteArray::ptr ba2(new wyz::ByteArray(base_len * 2)); \
    WYZ_ASSERT(ba2->readFromFile("/home/wyz/workspace/wyz/bytearray/" #type "_" #len "-" #read_fun ".dat")); \
    ba2->setPosition(0); \
    WYZ_ASSERT(ba->toString() == ba2->toString()); \
    WYZ_ASSERT(ba->getPosition() == 0); \
    WYZ_ASSERT(ba2->getPosition() == 0); \
}
    XX(int8_t,  100, writeFint8, readFint8, 1);
    XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(int16_t,  100, writeFint16,  readFint16, 1);
    XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(int32_t,  100, writeFint32,  readFint32, 1);
    XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    XX(int64_t,  100, writeFint64,  readFint64, 1);
    XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t,  100, writeInt32,  readInt32, 1);
    XX(uint32_t, 100, writeUint32, readUint32, 1);
    XX(int64_t,  100, writeInt64,  readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);

#undef XX

}

int main(){
    test_bytearray();
    return 0; 
}