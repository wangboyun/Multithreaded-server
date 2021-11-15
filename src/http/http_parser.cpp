/**
 * @file http_parser.cpp
 * @brief 请求与响应 解析
 * @author wyz (501826086@qq.com)
 * @version 1.0
 * @date 2021-11-14
 * 
 * @copyright Copyright (c) 2021  wyz
 * 
 */
#include "http.h"
#include "http11_parser.h"
#include "http_parser.h"
#include "httpclient_parser.h"
#include "../log.h"
#include "../config.h"
#include <cstring>

namespace wyz {
namespace http {

static wyz::Logger::ptr g_logger = WYZ_LOG_NAME("system");

static wyz::ConfigVar<uint64_t>::ptr g_http_request_buffer_size = wyz::Config::Lookup("http.request.buffer_size", (uint64_t)4 * 1024,  "http request buffer_size");
static wyz::ConfigVar<uint64_t>::ptr g_http_request_body_size = wyz::Config::Lookup("http.request.body_size", (uint64_t) 64 * 1024 * 1024, "http request body_size");

static wyz::ConfigVar<uint64_t>::ptr g_http_response_buffer_size = wyz::Config::Lookup("http.response.buffer_size", (uint64_t)4 * 1024,  "http response buffer_size");
static wyz::ConfigVar<uint64_t>::ptr g_http_response_body_size = wyz::Config::Lookup("http.response.body_size", (uint64_t) 64 * 1024 * 1024, "http response body_size");

static uint64_t http_request_buffer_size = 0;
static uint64_t http_request_max_body_size = 0;
static uint64_t http_response_buffer_size = 0;
static uint64_t http_response_max_body_size = 0;

namespace  {
    struct _RequestSizeIniter{
    _RequestSizeIniter(){
        http_request_buffer_size = g_http_request_buffer_size->getValue();
        http_request_max_body_size = g_http_request_body_size->getValue();
        http_response_buffer_size = g_http_response_buffer_size->getValue();
        http_response_max_body_size = g_http_response_body_size->getValue();

        g_http_request_buffer_size->addListener([](const uint64_t& ov , const uint64_t& nv){
            http_request_buffer_size = nv;
        });

        g_http_request_body_size->addListener([](const uint64_t& ov , const uint64_t& nv){
            http_request_max_body_size = nv;
        });

        g_http_response_buffer_size->addListener([](const uint64_t& ov , const uint64_t& nv){
            http_response_buffer_size = nv;
        });

        g_http_response_body_size->addListener([](const uint64_t& ov , const uint64_t& nv){
            http_response_max_body_size = nv;
        });
    }
};

static _RequestSizeIniter _requset_size_init;

}

/**
 * @brief 解析请求方法
 * @param  data             My Param doc
 * @param  at               My Param doc
 * @param  length           My Param doc
 */
void on_request_method(void *data, const char *at, size_t length){
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    HttpMethod m = CharsToHttpMethod(at, length);
    if(m == HttpMethod::INVALID_METHOD){
        WYZ_LOG_WARN(g_logger) << "request method unvaild";
        parser->setError(1000);
        return;
    }
    parser->getData()->setMethod(m);
}

void on_request_uri(void *data, const char *at, size_t length){

}

void on_request_fragment(void *data, const char *at, size_t length){
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setFragment(std::string(at,length));
}

void on_request_path(void *data, const char *at, size_t length){
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setPath(std::string(at,length));  
}

void on_request_query_string(void *data, const char *at, size_t length){
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setQuery(std::string(at,length));  
}

void on_http_request_version(void *data, const char *at, size_t length){
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    // uint8_t v = 0;
    if(strncmp(at,"HTTP/1.1",length) == 0){
        parser->getData()->setVersion(0x11);
    }else if(strncmp(at,"HTTP/1.0",length) == 0){
        parser->getData()->setVersion(0x10);
    }else {
        WYZ_LOG_WARN(g_logger) << "http request version is  unvaild";
        parser->setError(1001);
        return;
    }
}

void on_http_request_header_done(void *data, const char *at, size_t length){

}

void on_request_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen){
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    if(flen == 0){
        WYZ_LOG_ERROR(g_logger) << "http request file length == 0";
        return;
    }
    parser->getData()->setHeader(std::string(field ,flen), std::string(value ,vlen));
}


HttpRequestParser::HttpRequestParser()
    :m_error(0){
    m_data.reset(new HttpRequest());
    http_parser_init(&m_parser);
    m_parser.request_method = on_request_method;
    m_parser.request_uri = on_request_uri;
    m_parser.fragment = on_request_fragment;
    m_parser.request_path = on_request_path;
    m_parser.query_string = on_request_query_string;
    m_parser.http_version = on_http_request_version;
    m_parser.header_done = on_http_request_header_done;
    m_parser.http_field = on_request_http_field;
    m_parser.data = this;
}

int HttpRequestParser::isFinish(){
    return http_parser_finish(&m_parser);
}

int HttpRequestParser::hasError(){
    return m_error || http_parser_has_error(&m_parser);
}

size_t HttpRequestParser::exectue( char* data , size_t len){
    size_t offset = http_parser_execute(&m_parser,data , len , 0);
    memmove(data, data + offset, (len - offset));
    return 0;
}


void on_response_reason_phrase(void *data, const char *at, size_t length){

}

void on_response_status_code(void *data, const char *at, size_t length){

}

void on_response_chunk_size(void *data, const char *at, size_t length){

}

void on_response_http_version(void *data, const char *at, size_t length){

}

void on_response_header_done(void *data, const char *at, size_t length){

}

void on_response_last_chunk(void *data, const char *at, size_t length){

}

void on_response_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen){
    
}

HttpResponseParser::HttpResponseParser()
    : m_error(0){
    m_data.reset(new HttpResponse());
    httpclient_parser_init(&m_parser);
    m_parser.reason_phrase = on_response_reason_phrase;
    m_parser.status_code = on_response_status_code;
    m_parser.chunk_size = on_response_chunk_size;
    m_parser.http_version = on_response_http_version;
    m_parser.header_done = on_response_header_done;
    m_parser.last_chunk = on_response_last_chunk;
    m_parser.http_field = on_response_http_field;
    m_parser.data = this;
}

int HttpResponseParser::isFinish(){
    return httpclient_parser_finish(&m_parser);
}

int HttpResponseParser::hasError(){
    return m_error || httpclient_parser_has_error(&m_parser);
}
    
size_t HttpResponseParser::exectue(char* data , size_t len){
    return 0;
}

}
}