/**
 * @file http_session.h
 * @brief 
 * @author wyz (501826086@qq.com)
 * @version 1.0
 * @date 2021-11-19
 * 
 * @copyright Copyright (c) 2021  wyz
 * 
 */

#ifndef __WYZ_HTTP_SESSION_H__
#define __WYZ_HTTP_SESSION_H__

#include "http.h"
#include "../streams/socket_stream.h"

namespace wyz {
namespace http {

class HttpSession : public SocketStream {
public:
    using ptr = std::shared_ptr<HttpSession>;
    /**
     * @brief Construct a new Http Session object
     * @param  socket           socket套接字封装类
     * @param  owner            是否托管
     */
    HttpSession(Socket::ptr socket , bool owner = true);

    /**
     * @brief 接受客户端发来的 http 请求报文
     * @return HttpRequest::ptr 
     */
    HttpRequest::ptr recvRequest();

    /**
     * @brief 向客户端回复 http 响应报文
     * @param  rsp              http 响应报文
     * @return int              >0 发送成功
     *                          =0 对方关闭
     *                          <0 Socket异常
     */
    int sendResponse(HttpResponse::ptr rsp);

};

}
}


#endif