/**
 * @file bytearray.h
 * @brief 序列化：将数据结构或对象转换成二进制串的过程
 * 与反序列 
 * @author wyz (501826086@qq.com)
 * @version 1.0
 * @date 2021-11-08
 * 
 * @copyright Copyright (c) 2021  wyz
 * 
 */

#ifndef __WYZ_BYTEARRAY_H__
#define __WYZ_BYTEARRAY_H__

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <sys/uio.h>

namespace wyz {

class ByteArray{
public:
    using ptr = std::shared_ptr<ByteArray>;

    struct Node{
        Node();
        Node(size_t s);
        ~Node();

        /// 内存块地址指针
        char* ptr;
        /// 内存块大小
        size_t size;
        /// 下一个内存块地址
        Node* next;
    };

    /**
     * @brief 使用指定长度的内存块构造ByteArray
     * @param  base_size        内存块大小默认 4K
     */
    ByteArray(size_t base_size = 4096);
    ~ByteArray();

    /// write
    void writeFint8(int8_t value);
    void writeFuint8(uint8_t value);
    void writeFint16(int16_t value);
    void writeFuint16(uint16_t value);
    void writeFint32(int32_t value);
    void writeFuint32(uint32_t value);
    void writeFint64(int64_t value);
    void writeFuint64(uint64_t value);

    void writeInt32(int32_t value);
    void writeUint32(uint32_t value);
    void writeInt64(int64_t value);
    void writeUint64(uint64_t value);
    void writeFloat(float value);
    void writeDouble(double value);

    void writeStringF16(const std::string& value);
    void writeStringF32(const std::string& value);
    void writeStringF64(const std::string& value);
    void writeStringVint(const std::string& value);
    void writeStringWithoutLength(const std::string& value);

    /// read
    int8_t readFint8();
    uint8_t readFuint8();
    int16_t readFint16();
    uint16_t readFuint16();
    int32_t readFint32();
    uint32_t readFuint32();
    int64_t readFint64();
    uint64_t readFuint64();

    int32_t readInt32();
    uint32_t readUint32();
    int64_t readInt64();
    uint64_t readUint64();
    float readFloat();
    double readDouble();

    std::string readStringF16();
    std::string readStringF32();
    std::string readStringF64();
    std::string readStringVint();

    /**
     * @brief 清除所以结点数据
     */
    void clear();

    /**
     * @brief 将buf 中的数据 写入结点
     * @param  buf              待写入的数据
     * @param  size             写入数据大小
     */
    void write(const void* buf , size_t size);

    /**
     * @brief 将buf 数据读入结点里
     * @param  buf              待读入的数据 
     * @param  size             数据大小
     */
    void read(void* buf , size_t size);

    /**
     * @brief 将buf 数据读入结点里指定的位置上
     * @param  buf              待读入的数据 
     * @param  size             数据大小
     * @param  position         操作的位置
     */
    void read(void* buf , size_t size , size_t position) const;

    bool writeToFile(const std::string& name) const;
    bool readFromFile(const std::string& name);

    /**
     * @brief 获得字节序当前位置 m_position
     * @return size_t 
     */
    inline size_t getPosition() const    {return m_position;};

    /**
     * @brief 设置字节序当前位置(包括 m_position , m_cur)
     * @param  value            距离数据开始处的距离
     */
    void setPosition(size_t value);    

    /**
     * @brief 获得每一个节点的基础空间大小
     * @return size_t 
     */
    inline size_t getBaseSize()const    {return m_baseSize;};

    /**
     * @brief 获得可读的空间大小
     * @return size_t 
     */
    inline size_t getReadSize() const   {return m_size - m_position;}

    /**
     * @brief 获得可写的空间大小
     * @return size_t 
     */
    inline size_t getWriteSize() const  {return m_size - m_position;};

    /**
     * @brief 获得目前总的数据大小
     * @return size_t 
     */
    inline size_t getSize() const       {return m_size;};
    
    std::string toString() const;
    std::string toHexString() const;

    /**
     * @brief 获取可读取的缓存,保存成iovec数组
     * @param[out] buffers 保存可读取数据的iovec数组
     * @param[in] len 读取数据的长度,如果len > getReadSize() 则 len = getReadSize()
     * @return 返回实际数据的长度
     */
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull) const;

    /**
     * @brief 获取可读取的缓存,保存成iovec数组,从position位置开始
     * @param[out] buffers 保存可读取数据的iovec数组
     * @param[in] len 读取数据的长度,如果len > getReadSize() 则 len = getReadSize()
     * @param[in] position 读取数据的位置
     * @return 返回实际数据的长度
     */
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const;

    /**
     * @brief 获取可写入的缓存,保存成iovec数组
     * @param[out] buffers 保存可写入的内存的iovec数组
     * @param[in] len 写入的长度
     * @return 返回实际的长度
     * @post 如果(m_position + len) > m_capacity 则 m_capacity扩容N个节点以容纳len长度
     */
    uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);

private:

    void addCapacity(size_t size);
    size_t getCapacity() const  {return m_capacity - m_position;}

private:
    
    size_t m_baseSize;  /// 内存块的大小
    size_t m_position;  /// 当前操作位置
    size_t m_capacity;  /// 当前的总容量
    size_t m_size;      /// 当前数据的大小
    Node* m_root;       /// 第一个内存块指针
    Node* m_cur;        /// 当前操作的内存块指针

    // /// 字节序,默认大端
    // int8_t m_endian;
};

}


#endif