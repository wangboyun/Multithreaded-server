/**
 * @file http.h
 * @brief  http 协议封装
 * @author wyz (501826086@qq.com)
 * @version 1.0
 * @date 2021-11-11
 * 
 * @copyright Copyright (c) 2021  wyz
 * 
 */

#ifndef __WYZ_HTTP__
#define __WYZ_HTTP__

#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include <map>
#include <boost/lexical_cast.hpp>

namespace wyz {
namespace http {

/* Request Methods */
#define HTTP_METHOD_MAP(XX)         \
  XX(0,  DELETE,      DELETE)       \
  XX(1,  GET,         GET)          \
  XX(2,  HEAD,        HEAD)         \
  XX(3,  POST,        POST)         \
  XX(4,  PUT,         PUT)          \
  /* pathological */                \
  XX(5,  CONNECT,     CONNECT)      \
  XX(6,  OPTIONS,     OPTIONS)      \
  XX(7,  TRACE,       TRACE)        \
  /* WebDAV */                      \
  XX(8,  COPY,        COPY)         \
  XX(9,  LOCK,        LOCK)         \
  XX(10, MKCOL,       MKCOL)        \
  XX(11, MOVE,        MOVE)         \
  XX(12, PROPFIND,    PROPFIND)     \
  XX(13, PROPPATCH,   PROPPATCH)    \
  XX(14, SEARCH,      SEARCH)       \
  XX(15, UNLOCK,      UNLOCK)       \
  XX(16, BIND,        BIND)         \
  XX(17, REBIND,      REBIND)       \
  XX(18, UNBIND,      UNBIND)       \
  XX(19, ACL,         ACL)          \
  /* subversion */                  \
  XX(20, REPORT,      REPORT)       \
  XX(21, MKACTIVITY,  MKACTIVITY)   \
  XX(22, CHECKOUT,    CHECKOUT)     \
  XX(23, MERGE,       MERGE)        \
  /* upnp */                        \
  XX(24, MSEARCH,     M-SEARCH)     \
  XX(25, NOTIFY,      NOTIFY)       \
  XX(26, SUBSCRIBE,   SUBSCRIBE)    \
  XX(27, UNSUBSCRIBE, UNSUBSCRIBE)  \
  /* RFC-5789 */                    \
  XX(28, PATCH,       PATCH)        \
  XX(29, PURGE,       PURGE)        \
  /* CalDAV */                      \
  XX(30, MKCALENDAR,  MKCALENDAR)   \
  /* RFC-2068, section 19.6.1.2 */  \
  XX(31, LINK,        LINK)         \
  XX(32, UNLINK,      UNLINK)       \
  /* icecast */                     \
  XX(33, SOURCE,      SOURCE)       \


/* Status Codes */
#define HTTP_STATUS_MAP(XX)                                                 \
  XX(100, CONTINUE,                        Continue)                        \
  XX(101, SWITCHING_PROTOCOLS,             Switching Protocols)             \
  XX(102, PROCESSING,                      Processing)                      \
  XX(200, OK,                              OK)                              \
  XX(201, CREATED,                         Created)                         \
  XX(202, ACCEPTED,                        Accepted)                        \
  XX(203, NON_AUTHORITATIVE_INFORMATION,   Non-Authoritative Information)   \
  XX(204, NO_CONTENT,                      No Content)                      \
  XX(205, RESET_CONTENT,                   Reset Content)                   \
  XX(206, PARTIAL_CONTENT,                 Partial Content)                 \
  XX(207, MULTI_STATUS,                    Multi-Status)                    \
  XX(208, ALREADY_REPORTED,                Already Reported)                \
  XX(226, IM_USED,                         IM Used)                         \
  XX(300, MULTIPLE_CHOICES,                Multiple Choices)                \
  XX(301, MOVED_PERMANENTLY,               Moved Permanently)               \
  XX(302, FOUND,                           Found)                           \
  XX(303, SEE_OTHER,                       See Other)                       \
  XX(304, NOT_MODIFIED,                    Not Modified)                    \
  XX(305, USE_PROXY,                       Use Proxy)                       \
  XX(307, TEMPORARY_REDIRECT,              Temporary Redirect)              \
  XX(308, PERMANENT_REDIRECT,              Permanent Redirect)              \
  XX(400, BAD_REQUEST,                     Bad Request)                     \
  XX(401, UNAUTHORIZED,                    Unauthorized)                    \
  XX(402, PAYMENT_REQUIRED,                Payment Required)                \
  XX(403, FORBIDDEN,                       Forbidden)                       \
  XX(404, NOT_FOUND,                       Not Found)                       \
  XX(405, METHOD_NOT_ALLOWED,              Method Not Allowed)              \
  XX(406, NOT_ACCEPTABLE,                  Not Acceptable)                  \
  XX(407, PROXY_AUTHENTICATION_REQUIRED,   Proxy Authentication Required)   \
  XX(408, REQUEST_TIMEOUT,                 Request Timeout)                 \
  XX(409, CONFLICT,                        Conflict)                        \
  XX(410, GONE,                            Gone)                            \
  XX(411, LENGTH_REQUIRED,                 Length Required)                 \
  XX(412, PRECONDITION_FAILED,             Precondition Failed)             \
  XX(413, PAYLOAD_TOO_LARGE,               Payload Too Large)               \
  XX(414, URI_TOO_LONG,                    URI Too Long)                    \
  XX(415, UNSUPPORTED_MEDIA_TYPE,          Unsupported Media Type)          \
  XX(416, RANGE_NOT_SATISFIABLE,           Range Not Satisfiable)           \
  XX(417, EXPECTATION_FAILED,              Expectation Failed)              \
  XX(421, MISDIRECTED_REQUEST,             Misdirected Request)             \
  XX(422, UNPROCESSABLE_ENTITY,            Unprocessable Entity)            \
  XX(423, LOCKED,                          Locked)                          \
  XX(424, FAILED_DEPENDENCY,               Failed Dependency)               \
  XX(426, UPGRADE_REQUIRED,                Upgrade Required)                \
  XX(428, PRECONDITION_REQUIRED,           Precondition Required)           \
  XX(429, TOO_MANY_REQUESTS,               Too Many Requests)               \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
  XX(451, UNAVAILABLE_FOR_LEGAL_REASONS,   Unavailable For Legal Reasons)   \
  XX(500, INTERNAL_SERVER_ERROR,           Internal Server Error)           \
  XX(501, NOT_IMPLEMENTED,                 Not Implemented)                 \
  XX(502, BAD_GATEWAY,                     Bad Gateway)                     \
  XX(503, SERVICE_UNAVAILABLE,             Service Unavailable)             \
  XX(504, GATEWAY_TIMEOUT,                 Gateway Timeout)                 \
  XX(505, HTTP_VERSION_NOT_SUPPORTED,      HTTP Version Not Supported)      \
  XX(506, VARIANT_ALSO_NEGOTIATES,         Variant Also Negotiates)         \
  XX(507, INSUFFICIENT_STORAGE,            Insufficient Storage)            \
  XX(508, LOOP_DETECTED,                   Loop Detected)                   \
  XX(510, NOT_EXTENDED,                    Not Extended)                    \
  XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required) \

/**
 * @brief HTTP方法枚举
 */
enum class HttpMethod {
#define XX(num, name, string) name = num,
    HTTP_METHOD_MAP(XX)
#undef XX
    INVALID_METHOD
};

/**
 * @brief HTTP状态枚举
 */
enum class HttpStatus {
#define XX(code, name, desc) name = code,
    HTTP_STATUS_MAP(XX)
#undef XX
};

HttpMethod StringToHttpMethod(const std::string& m);
HttpMethod CharsToHttpMethod(const char* m , size_t len);
const char* HttpMethodToString(HttpMethod& m);
const char* HttpStatusToString(HttpStatus& s);


/**
 * @brief 忽略大小写的比较仿函数
 */
struct CaseInsensitiveLess{
    bool operator() (const std::string& lhs , const std::string& rhs) const;
};

/**
 * @brief  获取Map中的key值,并转成对应类型,返回是否成功
 * @tparam MapType  
 * @tparam T 
 * @param  m                Map数据结构
 * @param  key              关键字
 * @param  val              保存转换后的值
 * @param  def              默认值
 * @return true             转换成功, val 为对应的值
 * @return false            不存在或者转换失败 val = def
 */
template <typename MapType, typename T>
bool checkGetAs(const MapType& m,const std::string& key, T& val , const T& def = T()){
    auto it = m.find(key);
    if(it == m.end()){
        val = def;
        return false;
    }
    try {
        val = boost::lexical_cast<T>(it->second);
        return true;
    } catch (...) {
        val = def;
    }
    return false;
}

/**
 * @brief 获取Map中的key值,并转成对应类型
 * @tparam MapType 
 * @tparam T 
 * @param  m                Map数据结构
 * @param  key              关键字
 * @param  def              默认值
 * @return T           如果存在且转换成功返回对应的值,否则返回默认值
 */
template <typename MapType, typename T>
T getAs(const MapType& m,const std::string& key , const T& def = T()){
    auto it = m.find(key);
    if(it == m.end()){
        return def;
    }
    try {
        return boost::lexical_cast<T>(it->second);
    } catch (...) {
    }
    return def;
}


/**
 * @brief http 请求结构
 * https://cn.bing.com/search?q=github&form=ANNTH1&refig=c117ffffbf524b03b499e5971e99965a#index
 * scheme:[//[user:password@]host[:port]][/]path[?query][#fragment]
 */
class HttpRequest{
public:
    using ptr = std::shared_ptr<HttpRequest>;
    using MapType = std::map<std::string, std::string, CaseInsensitiveLess> ;

    HttpRequest(uint8_t version = 0x11 , bool close = true);

    /**
     * @brief 一些列参数的 get 函数
     */
    inline HttpMethod getMethod() const             {return m_method;}
    inline uint8_t    getVersion()const             {return m_version;} 
    inline bool isClose() const     {return m_close;}

    inline const std::string& getPath()  const      {return m_path;}
    inline const std::string& getQuery() const      {return m_query;}
    inline const std::string& getFragment() const   {return m_fragment;}
    inline const std::string& getBody() const       {return m_body;}

    inline const MapType& getHeaders() const        {return m_headers;}
    inline const MapType& getPararms() const        {return m_pararms;}
    inline const MapType& getCookies() const        {return m_cookies;}

    std::string getHeader(const std::string& key , const std::string& def = "") const;

    std::string getPararm(const std::string& key, const std::string& def = "") const;

    std::string getCookie(const std::string& key, const std::string& def = "") const;

    /**
     * @brief 一些列参数的 set 函数
     */
    inline void setMethod(const HttpMethod& v)      {m_method = v;}
    inline void setVersion(const uint8_t v)         {m_version = v;}
    inline void setClose(bool v)                    {m_close = v;}

    inline void setPath(const std::string & v)      {m_path = v;}
    inline void setQuery(const std::string& v)      {m_query = v;}
    inline void setFragment(const std::string& v)   {m_fragment = v;}
    inline void setBody(const std::string& v)       {m_body = v;}
    
    inline void setHeaders(const MapType& v)        {m_headers = v;}
    inline void setPararms(const MapType& v)        {m_pararms = v;}
    inline void setCookies(const MapType& v)        {m_cookies = v;}
    
    void setHeader(const std::string& key , const std::string& val);

    void setPararm(const std::string& key , const std::string& val);

    void setCookie(const std::string& key , const std::string& val);

    /**
     * @brief  MapType 相关的删除操作
     */
    void delHeader(const std::string& key);
    void delPararm(const std::string& key);
    void delCookie(const std::string& key);

    /**
     * @brief  判断HTTP请求的头部参数(key)是否存在
     * @param  key              参数key 关键字
     * @param  val              val 如果存在,val非空则赋值
     * @return true             存在
     * @return false            不存在
     */
    bool hasHeader(const std::string& key, std::string* val = nullptr);
    bool hasPararm(const std::string& key, std::string* val = nullptr);
    bool hasCookie(const std::string& key, std::string* val = nullptr);

    template<typename T>
    bool checkGetHeaderAs(const std::string& key , T& val , const T& def = T()){
        return checkGetAs(m_headers, key, val,def);
    }

    template<typename T>
    T getHeaderAs(const std::string& key , const T& def = T()){
        return getAs(m_headers, key, def);
    }


    template<typename T>
    bool checkGetPararmAs(const std::string& key , T& val , const T& def = T()){
        initQueryParam();
        initBodyParam();
        return checkGetAs(m_headers, key, val,def);
    }

    template<typename T>
    T getPararmAs(const std::string& key , const T& def = T()){
        initQueryParam();
        initBodyParam();
        return getAs(m_headers, key,def);
    }

    template<typename T>
    bool checkGetCookieAs(const std::string& key , T& val , const T& def = T()){
        initCookies();
        return checkGetAs(m_headers, key, val,def);
    }

    template<typename T>
    T getCookieAs(const std::string& key , const T& def = T()){
        initCookies();
        return getAs(m_headers, key, def);
    }

    std::ostream& dump(std::ostream& os);
    std::string toString();
    void init();
    void initParam();
    void initQueryParam();
    void initBodyParam();
    void initCookies();

private:
    HttpMethod m_method;        /// http 方式
    uint8_t m_version;          /// http 版本
    bool m_close;               /// 是否自动关闭

    std::string m_path;         /// 请求路径
    std::string m_query;        /// 请求参数
    std::string m_fragment;     /// 请求fragment
    std::string m_body;         /// 请求消息体

    MapType m_headers;          /// 请求头部
    MapType m_pararms;          /// 请求参数
    MapType m_cookies;          /// 请求cookie
};

class HttpResponse{
public:
    using ptr = std::shared_ptr<HttpResponse>;
    using MapType = std::map<std::string, std::string, CaseInsensitiveLess> ;

    HttpResponse(uint8_t version = 0x11 , bool close = true);

    /**
     * @brief 一系列get / set 函数
     */
    inline HttpStatus getStatus() const     {return m_status;}
    inline uint8_t getVersion() const       {return m_version;}
    inline bool isClose() const             {return m_close;}
    inline std::string getBody() const      {return m_body;}
    inline std::string getReason() const    {return m_reason;}
    inline MapType getHeader()  const       {return m_headers;}
    std::string getHeader(const std::string& key , const std::string& def = "") const;

    inline void setStatus(const HttpStatus& v)      {m_status = v;}
    inline void setVersion(uint8_t v)               {m_version = v;}
    inline void setClose(bool v)                    {m_close = v;}
    inline void setBody(const std::string& v)       {m_body = v;}
    inline void setReason(const std::string& v)     {m_reason = v;}
    inline void setHeader(const MapType& v)         {m_headers = v;}

    void setHeader(const std::string& key , const std::string& val);
    void delHeader(const std::string& key);

    template<typename T>
    bool checkGetHeaderAs(const std::string& key , T& val , const T& def = T()){
        return checkGetAs(m_headers, key, val,def);
    }

    template<typename T>
    T getHeaderAs(const std::string& key , const T& def = T()){
        return getAs(m_headers, key,def);
    }

    std::ostream& dump(std::ostream& os);
    std::string toString();
private:
    HttpStatus m_status;        /// 回应的状态
    uint8_t m_version;          /// http 版本号
    bool m_close;               /// 是否自动关闭

    std::string m_body;         /// 回应消息体
    std::string m_reason;       /// 回应的解释

    MapType m_headers;          /// 回应头部
};

}
}



#endif