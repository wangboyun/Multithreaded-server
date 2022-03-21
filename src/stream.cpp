#include "stream.h"
#include <cstddef>

namespace wyz {

int Stream::readFixSize(void * buff , size_t length){
    size_t left = length;
    size_t offset = 0;
    while(left > 0){
        int len = read((char*) buff + offset , left);
        if(len <= 0){
            return len;
        }
        offset += len;
        left -= len;
    }
    return length;
}

int Stream::readFixSize(ByteArray::ptr ba , size_t length){
    size_t left = length;
    while(left > 0){
        int len = read(ba , left);
        if(len <= 0){
            return len;
        }
        left -= len;
    }
    return length;
}

int Stream::writeFixSize(const void * buff , size_t length){
    size_t left = length;
    size_t offset = 0;
    while(left > 0){
        int len = write((char*) buff + offset , left);
        if(len <= 0){
            return len;
        }
        left -= len;
        offset += len;
    }
    return length;
}

int Stream::writeFixSize(ByteArray::ptr ba , size_t length){
    size_t left = length;
    while(left > 0){
        int len = write(ba, left);
        if(len <= 0){
            return len;
        }
        left -= len;
    }
    return length;
}


}