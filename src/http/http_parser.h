/**
 * @file http_parser.h
 * @brief  解析 http 协议解析封装
 * @author wyz (501826086@qq.com)
 * @version 1.0
 * @date 2021-11-14
 * 
 * @copyright Copyright (c) 2021  wyz
 * 
 */

#ifndef __HTTP_PARSER_H__
#define __HTTP_PARSER_H__

#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"
#include <cstdint>

namespace wyz {
namespace http {

class HttpRequestParser{
public:
    using ptr = std::shared_ptr<HttpRequestParser>;
    HttpRequestParser();

    /**
     * @brief 是否解析完成
     * @return int 
     */
    int isFinish();

    /**
     * @brief 解析是否有错
     * @return int 
     */
    int hasError();
    
    /**
     * @brief 解析协议
     * @param  data            协议文本内存
     * @param  len             协议文本内存长度
     * @return size_t 返回实际解析的长度,并且将已解析的数据移除
     */
    size_t exectue( char* data , size_t len);

    inline HttpRequest::ptr getData() const      {return m_data;}

    inline void setError(int v) {m_error = v;}

    uint64_t getContentLength();

public:
    static uint64_t GetHttpRequestBufferSize();
    static uint64_t GetHttpRequestBodySize();
private:
    http_parser m_parser;         
    HttpRequest::ptr m_data;    /// 请求数据
    int m_error;
};

class HttpResponseParser{
public:
    using ptr = std::shared_ptr<HttpResponseParser>;
    HttpResponseParser();

    /**
     * @brief 是否解析完成
     * @return int 
     */
    int isFinish();

    /**
     * @brief 解析是否有错
     * @return int 
     */
    int hasError();
    
    /**
     * @brief 解析协议
     * @param  data            协议文本内存
     * @param  len             协议文本内存长度
     * @return size_t 返回实际解析的长度,并且将已解析的数据移除
     */
    size_t exectue(char* data , size_t len);

    inline HttpResponse::ptr getData() const      {return m_data;}

    inline void setError(int v) {m_error = v;}

    uint64_t getContentLength();

public:
    static uint64_t GetHttpResponseBufferSize();
    static uint64_t GetHttpResponseBodySize();

private:
    httpclient_parser m_parser;         
    HttpResponse::ptr m_data;    /// 接受数据
    int m_error;
};


}
}

#endif