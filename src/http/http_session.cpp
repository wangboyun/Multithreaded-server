/**
 * @file http_session.cpp
 * @brief httpsession 封装实现
 * @author wyz (501826086@qq.com)
 * @version 1.0
 * @date 2021-11-19
 * 
 * @copyright Copyright (c) 2021  wyz
 * 
 */

#include "http_session.h"
#include "http_parser.h"
#include <cstdint>
#include <memory>

namespace wyz {
namespace http {

HttpSession::HttpSession(Socket::ptr socket , bool owner )
    : SocketStream(socket, owner){}

HttpRequest::ptr HttpSession::recvRequest(){
    HttpRequestParser::ptr parser(new HttpRequestParser);
    uint64_t buffsize = HttpRequestParser::GetHttpRequestBufferSize();
    std::shared_ptr<char> buffer(new char[buffsize] , [](char* ptr){
        delete [] ptr;      /// 自定义的析构方法
    });
    char* data = buffer.get();
    int offset = 0;
    do {
        int len = read(data + offset , buffsize - offset);
        if(len < 0){
            close();
            return nullptr;
        }
        len += offset;
        size_t nparser = parser->exectue(data, len);
        if(parser->hasError()){
            close();
            return nullptr;
        }
        offset = len - nparser;
        if(offset == (int) buffsize){
            close();
            return nullptr;
        }
        if(parser->isFinish()){
            break;
        }

    }while (true);

    uint64_t length = parser->getContentLength();
    if(length > 0){
        std::string body;
        body.resize(length);
        int len = 0;
        if((int)length >= offset) {
            memcpy(&body[0], data, offset);
            len = offset;
        } else {
            memcpy(&body[0], data, length);
            len = length;
        }
        length -= offset;
        if(length > 0) {
            if(readFixSize(&body[len], length) <= 0) {
                close();
                return nullptr;
            }
        }
        parser->getData()->setBody(body);
    }

    parser->getData()->init();
    return parser->getData();
    
}

int HttpSession::sendResponse(HttpResponse::ptr rsp){
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}



}
}