/**
 * @file bytearray.cpp
 * @brief 序列化与反序列化
 * @author wyz (501826086@qq.com)
 * @version 1.0
 * @date 2021-11-09
 * 
 * @copyright Copyright (c) 2021  wyz
 * 
 */

#include "bytearray.h"
#include "log.h"
#include <arpa/inet.h>
#include <bits/types/struct_iovec.h>
#include <cstddef>
#include <cstring>
#include <endian.h>
#include <fstream>
#include <iomanip>
#include <netinet/in.h>
#include <string>


namespace wyz {

static wyz::Logger::ptr g_logger = WYZ_LOG_NAME("system");

ByteArray::Node::Node()
    : ptr(nullptr)
    , size(0)
    , next(nullptr){

}

ByteArray::Node::Node(size_t s)
    : ptr(new char[s])
    , size(s)
    , next(nullptr){

}

ByteArray::Node::~Node(){
    if(ptr){
        delete [] ptr;
    }

}

ByteArray::ByteArray(size_t base_size)
    : m_baseSize(base_size)
    , m_position(0)
    , m_capacity(base_size)
    , m_size(0)
    , m_root(new Node(base_size))
    , m_cur(m_root){

}

ByteArray::~ByteArray(){
    Node* temp = m_root;
    while(temp){
        m_cur = temp;
        temp = temp->next;
        delete m_cur;
    }
}

void ByteArray::writeFint8(int8_t value){
    write(&value, 1);
}

void ByteArray::writeFuint8(uint8_t value){
    write(&value, sizeof(value));
}

void ByteArray::writeFint16(int16_t value){
    /// 会有字节序的要求
    int16_t temp =  htons(value);
    write(&temp, sizeof(value));
}

void ByteArray::writeFuint16(uint16_t value){
    uint16_t temp =  htons(value);
    write(&temp, sizeof(value));
}

void ByteArray::writeFint32(int32_t value){
    int32_t temp = htonl(value);
    write(&temp, sizeof(value));
}

void ByteArray::writeFuint32(uint32_t value){
    uint32_t temp = htonl(value);
    write(&temp, sizeof(value));
}

void ByteArray::writeFint64(int64_t value){
    int64_t temp = htobe64(value);
    write(&temp, sizeof(value));
}

void ByteArray::writeFuint64(uint64_t value){
    uint64_t temp = htobe64(value);
    write(&temp, sizeof(value));
}

static uint32_t EncodeZigzag32(const int32_t& v){
    if(v < 0){
        return ((uint32_t) (-v) << 1) - 1;
    }else {
        return v << 1;
    }
}

static uint64_t EncodeZigzag64(const int64_t& v){
    if(v < 0){
        return ((uint64_t) (-v)  << 1) - 1;
    }else {
        return v  << 1;
    }
}

static int32_t DecodeZigzag32(const uint32_t& v) {
    return (v >> 1) ^ -(v & 1);
}

static int64_t DecodeZigzag64(const uint64_t& v) {
    return (v >> 1) ^ -(v & 1);
}

void ByteArray::writeInt32(int32_t value){
    writeUint32(EncodeZigzag32(value));
}

void ByteArray::writeUint32(uint32_t value){
    uint8_t tmp[5];
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}

void ByteArray::writeInt64(int64_t value){
    writeUint64(EncodeZigzag32(value));
}

void ByteArray::writeUint64(uint64_t value){
    uint8_t tmp[10];
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}

void ByteArray::writeFloat(float value){
    uint32_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint32(v);
}

void ByteArray::writeDouble(double value){
    uint64_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint64(v);
}

void ByteArray::writeStringF16(const std::string& value){
    writeFuint16(value.size());
    write(value.c_str(),value.size());
}

void ByteArray::writeStringF32(const std::string& value){
    writeFuint32(value.size());
    write(value.c_str(),value.size());
}
void ByteArray::writeStringF64(const std::string& value){
    writeFuint64(value.size());
    write(value.c_str(),value.size());
}

void ByteArray::writeStringVint(const std::string& value){
    writeUint64(value.size());
    write(value.c_str(), value.size());
}
    
void ByteArray::writeStringWithoutLength(const std::string& value){
    write(value.c_str(), value.size());
}

int8_t ByteArray::readFint8(){
    int8_t v;
    read(&v, sizeof(v));
    return v;
}

uint8_t ByteArray::readFuint8(){
    u_int8_t v;   
    read(&v, sizeof(v));
    return v;
}

int16_t ByteArray::readFint16(){
    int16_t v;
    read(&v, sizeof(v));
    return ntohs(v);
}

uint16_t ByteArray::readFuint16(){
    uint16_t v;
    read(&v, sizeof(v));
    return ntohs(v);
}

int32_t ByteArray::readFint32(){
    int32_t v;
    read(&v, sizeof(v));
    return ntohl(v);
}
uint32_t ByteArray::readFuint32(){
    uint32_t v;
    read(&v, sizeof(v));
    return ntohl(v);
}

int64_t ByteArray::readFint64(){
    int64_t v;
    read(&v, sizeof(v));
    return be64toh(v);
}

uint64_t ByteArray::readFuint64(){
    uint64_t v;
    read(&v, sizeof(v));
    return be64toh(v);
}

int32_t ByteArray::readInt32(){
    return DecodeZigzag32(readUint32());
}

uint32_t ByteArray::readUint32(){
    uint32_t result = 0;
    for(int i = 0; i < 32; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            result |= ((uint32_t)b) << i;
            break;
        } else {
            result |= (((uint32_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

int64_t ByteArray::readInt64(){
     return DecodeZigzag64(readUint64());
}

uint64_t ByteArray::readUint64(){
    uint64_t result = 0;
    for(int i = 0; i < 64; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            result |= ((uint64_t)b) << i;
            break;
        } else {
            result |= (((uint64_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

float ByteArray::readFloat(){
    uint32_t v = readFuint32();
    float value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

double ByteArray::readDouble(){
    uint64_t v = readFuint64();
    double value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

std::string ByteArray::readStringF16(){
    uint16_t len = readFuint16();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringF32(){
    uint32_t len = readFuint32();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringF64(){
    uint64_t len = readFuint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringVint(){
    uint64_t len = readUint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

void ByteArray::clear(){
    m_position = m_size = 0;
    m_capacity = m_baseSize;
    Node* temp = m_root->next;
    while(temp){
        m_cur = temp;
        temp = temp->next;
        delete m_cur;
    }
    m_cur = m_root;
    m_root->next = nullptr;
}

void ByteArray::write(const void* buf , size_t size){
    if(size == 0){
        return ;
    }
    addCapacity(size);

    /// 算出当前节点 cur 被占的空间 
    size_t npos = m_position % m_baseSize;
    /// 算出当前节点剩余空间大小
    size_t ncap = m_cur->size - npos;
    /// buf 里被写入了多少数据 
    size_t bpos = 0;

    while(size > 0){
        if(ncap >= size){
            memcpy(m_cur->ptr + npos, (const char*) buf+ bpos, size);
            if(m_cur->size == (npos + size)){
                m_cur = m_cur->next;
            }
            m_position += size;
            bpos += size;
            size = 0;
        }else {
            memcpy(m_cur->ptr + npos, (const char*) buf+ bpos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;
        }
    }
    if(m_position > m_size){
        m_size = m_position;
    }
}

void ByteArray::read(void* buf , size_t size){
    if(size > getReadSize()){
        throw std::out_of_range("not enough len");
    }
    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;
    while(size > 0) {
        if(ncap >= size) {
            memcpy((char*)buf + bpos, m_cur->ptr + npos, size);
            if(m_cur->size == (npos + size)) {
                m_cur = m_cur->next;
            }
            m_position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy((char*)buf + bpos, m_cur->ptr + npos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;
        }
    }
}

void ByteArray::read(void* buf , size_t size , size_t position) const{
    if(size > (m_size - position)){
        throw std::out_of_range("not enough len");
    }

    /// 根据指定的读取位置，找到要操作的节点 Node cur
    Node* cur = m_root;
    while(position > cur->size){
        position -= cur->size;
        cur = cur->next;
    }
    if(position == cur->size){
        cur = cur->next;
    }
    size_t npos = position % m_baseSize;
    size_t ncap = cur->size - npos;
    size_t bpos = 0;
    while(size > 0) {
        if(ncap >= size) {
            memcpy((char*)buf + bpos, cur->ptr + npos, size);
            if(cur->size == (npos + size)) {
                cur = cur->next;
            }
            position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy((char*)buf + bpos, cur->ptr + npos, ncap);
            position += ncap;
            bpos += ncap;
            size -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
    }
}

bool ByteArray::writeToFile(const std::string& name) const{
    std::ofstream ofs;
    /// trunc -- 清空 ， binary -- 二进制
    ofs.open(name,std::ios::trunc | std::ios::binary);
    if(!ofs){
        WYZ_LOG_ERROR(g_logger) << "writeToFile name=" << name << " error , errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    int64_t read_size = getReadSize();
    size_t position = m_position;
    Node* cur = m_cur;

    while(read_size > 0){
        int diff = position % m_baseSize;
        int64_t len = (read_size > (int64_t)m_baseSize ? m_baseSize : read_size) - diff;
        ofs.write(cur->ptr + diff, len);
        cur = cur->next;
        position += len;
        read_size -= len;
    }
    ofs.close();
    return true;
}

bool ByteArray::readFromFile(const std::string& name){
    std::ifstream ifs;
    ifs.open(name , std::ios::binary);
    if(!ifs){
        WYZ_LOG_ERROR(g_logger) << "writeToFile name=" << name << " error , errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    std::shared_ptr<char> buff(new char[m_baseSize] , [](char * ptr){ delete [] ptr;});

    while(!ifs.eof()){
        ifs.read(buff.get(), m_baseSize);
        write(buff.get(), ifs.gcount());
    }
    ifs.close();
    return true;
}



void ByteArray::setPosition(size_t value){
    if(value > m_capacity){
        throw std::out_of_range("set_position out of range");
    }
    m_position = value;

    m_cur = m_root;
    while(value > m_cur->size){
        value -= m_cur->size;
        m_cur = m_cur->next;
    }
    if(value == m_cur->size){
        m_cur = m_cur->next;
    }
}

std::string ByteArray::toString() const{
    std::string str;
    str.resize(getReadSize());
    if(str.empty()){
        return str;
    }
    size_t size = str.size();
    read(&str[0] , size , m_position);
    return str;
}

std::string ByteArray::toHexString() const{
    std::string str = toString();
    std::stringstream ss;

    for(size_t i = 0; i < str.size(); ++i) {
        if(i > 0 && i % 32 == 0) {
            ss << std::endl;
        }
        ss << std::setw(2) << std::setfill('0') << std::hex
           << (int)(uint8_t)str[i] << " ";
    }

    return ss.str();
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len ) const{
    len = (len > getReadSize()) ? getReadSize() : len;
    if(len == 0){
        return 0;
    }
    size_t size = len;
    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;

    struct iovec iov;
    Node* cur = m_cur;

    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = m_cur->size;
            npos = 0;
        }
        buffers.emplace_back(iov);
    }
    return size;
}


uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const{
    len = (len > getReadSize()) ? getReadSize() : len;
    if(len == 0){
        return 0;
    }
    size_t size = len;
    size_t npos = position % m_baseSize;

    Node* cur = m_root;
    while(position > cur->size){
        position -= cur->size;
        cur = cur->next;
    }
    if(position == cur->size){
        cur = cur->next;
    }

    size_t ncap = cur->size - npos;
    struct iovec iov;

    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = m_cur->size;
            npos = 0;
        }
        buffers.emplace_back(iov);
    }
    return size;
}

uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len){
    if(len == 0){
        return 0;
    }
    addCapacity(len);
    size_t size = len;

    /// 算出当前节点 cur 被占的空间 
    size_t npos = m_position % m_baseSize;
    /// 算出当前节点剩余空间大小
    size_t ncap = m_cur->size - npos;
    /// buf 里被写入了多少数据 
    iovec iov;
    Node* cur = m_cur;
    while(len > 0){
        if(ncap >= len){
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        }else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.emplace_back(iov);
    }
    return size;
}

void ByteArray::addCapacity(size_t value){
    if(value <= 0){
        return ;
    }
    size_t old_cap = getCapacity();
    if(value <= old_cap){
        return;
    }
    value = value - old_cap;
    int count = value / m_baseSize + ((value % m_baseSize > old_cap) ? 1 : 0) ;

    Node* temp = m_root;
    while(temp->next){
        temp = temp->next;
    }

    Node* first = NULL;
    for(int i = 0; i < count; ++i) {
        temp->next = new Node(m_baseSize);
        if(first == NULL) {
            first = temp->next;
        }
        temp = temp->next;
        m_capacity += m_baseSize;
    }

    if(old_cap == 0) {
        m_cur = first;
    }
}


}