
#include "http.h"
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <strings.h>

namespace wyz {
namespace http {

HttpMethod StringToHttpMethod(const std::string& m){
#define XX(num , name , string)\
    if(strcmp(#string , m.c_str()) == 0) { \
        return HttpMethod::name; \
    }
    HTTP_METHOD_MAP(XX);
#undef XX   
    return HttpMethod::INVALID_METHOD;
}

HttpMethod CharsToHttpMethod(const char* m , size_t len){
#define XX(num , name , string) \
    if(strncmp(#string, m , len) == 0) { \
        return HttpMethod::name; \
    }
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

static const char* s_method_string[] = {
#define XX(num , name , string) #string,
    HTTP_METHOD_MAP(XX)
#undef XX
};

const char* HttpMethodToString(HttpMethod& m){
    uint32_t index = (uint32_t) m;
    if(index >= (sizeof(s_method_string) / sizeof(s_method_string[0]))){
        return "<unknown>";
    }
    return s_method_string[index];
}

const char* HttpStatusToString(HttpStatus& s){
    switch (s) {
#define XX(code , name , msg) \
        case HttpStatus::name: \
            return #msg;
        HTTP_STATUS_MAP(XX);
#undef XX
        default:
            return "<unknown>";
    }
}

/**
 * @brief 忽略大小写的比较仿函数
 */
bool CaseInsensitiveLess::operator() (const std::string& lhs , const std::string& rhs) const{
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}


HttpRequest::HttpRequest(uint8_t version, bool close )
    : m_method(HttpMethod::GET)
    , m_version(version)
    , m_close(close)
    , m_path("/ "){

}

std::string HttpRequest::getHeader(const std::string& key , const std::string& def) const{
    auto it = m_headers.find(key);
    if(it == m_headers.end()){
        return def;
    }
    return it->second;
}

std::string HttpRequest::getPararm(const std::string& key, const std::string& def ) const{
    auto it = m_pararms.find(key);
    if(it == m_pararms.end()){
        return def;
    }
    return it->second;
}

std::string HttpRequest::getCookie(const std::string& key, const std::string& def ) const{
    auto it = m_cookies.find(key);
    if(it == m_cookies.end()){
        return def;
    }
    return it->second;
}

void HttpRequest::setHeader(const std::string& key , const std::string& val){
    m_headers[key] = val;
}

void HttpRequest::setPararm(const std::string& key , const std::string& val){
    m_pararms[key] = val;
}

void HttpRequest::setCookie(const std::string& key , const std::string& val) {
    m_cookies[key] = val;
}

void HttpRequest::delHeader(const std::string& key){
    m_headers.erase(key);
}

void HttpRequest::delPararm(const std::string& key){
    m_pararms.erase(key);
}

void HttpRequest::delCookie(const std::string& key){
    m_cookies.erase(key);
}

bool HttpRequest::hasHeader(const std::string& key, std::string* val){
    auto it = m_headers.find(key);
    if(it == m_headers.end()){
        return false;
    }
    if(val){
        *val = it->second;
    }
    return true;
}

bool HttpRequest::hasPararm(const std::string& key, std::string* val){
    initQueryParam();
    initBodyParam();
    auto it = m_pararms.find(key);
    if(it == m_pararms.end()){
        return false;
    }
    if(val){
        *val = it->second;
    }
    return true;
}

bool HttpRequest::hasCookie(const std::string& key, std::string* val){
    initCookies();
    auto it = m_cookies.find(key);
    if(it == m_cookies.end()){
        return false;
    }
    if(val){
        *val = it->second;
    }
    return true;
}

std::ostream& HttpRequest::dump(std::ostream& os){
    //GET /uri HTTP/1.1
    //Host: wwww.sylar.top
    //
    //
    os  << HttpMethodToString(m_method) << " "
        << m_path 
        << (m_query.empty() ? "" : "?" )
        << m_query 
        << (m_fragment.empty() ? "" : "#")
        << m_fragment
        << "HTTP/" 
        << ((uint32_t)(m_version >> 4))
        << "."
        << ((uint32_t)(m_version & 0x0F))
        << "\r\n";

    os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    for(auto i : m_headers){
        if(strcasecmp(i.first.c_str(), "connection") == 0)
            continue;
        os << i.first << ":" << i.second << "\r\n";
    }
    
    if(!m_body.empty()) {
        os << "content-length: " << m_body.size() << "\r\n\r\n"
           << m_body;
    } else {
        os << "\r\n";
    }
    return os;
}

std::string HttpRequest::toString(){
    std::stringstream ss;
    dump(ss);
    return ss.str();
}


void HttpRequest::init(){}
void HttpRequest::initParam(){}
void HttpRequest::initQueryParam(){}
void HttpRequest::initBodyParam(){}
void HttpRequest::initCookies(){}


HttpResponse::HttpResponse(uint8_t version  , bool close )
    : m_status(HttpStatus::OK)
    , m_version(version)
    , m_close(close)
    {

}

std::string HttpResponse::getHeader(const std::string& key , const std::string& def ) const{
    auto it = m_headers.find(key);
    if(it == m_headers.end()){
        return def;
    }else {
        return it->second;
    }
}

void HttpResponse::setHeader(const std::string& key , const std::string& val){
    m_headers[key] = val;
}

void HttpResponse::delHeader(const std::string& key){
    m_headers.erase(key);
}

std::ostream& HttpResponse::dump(std::ostream& os){
    os << "HTTP/"
        << ((uint32_t)(m_version >> 4))
        << "."
        << ((uint32_t)(m_version & 0x0F))
        << " "
        << (uint32_t) m_status
        << " "
        << (m_reason.empty() ? HttpStatusToString(m_status) : m_reason)
        << "\r\n";
    
    for(auto i : m_headers){
        if(strcasecmp(i.second.c_str() , "connection") == 0){
            continue;
        }
        os << i.first << ":" << i.second << "\r\n";
    }
    os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    if(!m_body.empty()) {
        os << "content-length: " << m_body.size() << "\r\n\r\n"
           << m_body;
    } else {
        os << "\r\n";
    }
    return os;
}

std::string HttpResponse::toString(){
    std::stringstream ss;
    dump(ss);
    return ss.str();
}


}
}