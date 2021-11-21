/**
 * @file http_server.cpp
 * @brief 
 * @author wyz (501826086@qq.com)
 * @version 1.0
 * @date 2021-11-20
 * 
 * @copyright Copyright (c) 2021  wyz
 * 
 */
#include "http_server.h"
#include "http.h"
#include "http_session.h"
#include "../log.h"

namespace wyz {
namespace http {

static wyz::Logger::ptr g_logger = WYZ_LOG_NAME("system");

HttpServer::HttpServer(bool keepalive , IOManager* worker , IOManager* acceptworker )
    : TCPServer(worker , acceptworker)
    , m_iskeepalive(keepalive){

}

void HttpServer::handleClient(Socket::ptr client) {
    WYZ_LOG_DEBUG(g_logger) << "handleClient client= " << *client;
    HttpSession::ptr session(new HttpSession(client));
    do {
        HttpRequest::ptr req = session->recvRequest();
        if(!req){
            /// 无 http 请求报文打印日志
            WYZ_LOG_ERROR(g_logger) << "recv http request fail, errno= "<< errno << " errstr= " << strerror(errno) << " cliet:" << *client << " keep_alive=" << m_iskeepalive;
            break;
        }
        HttpResponse::ptr rsp(new HttpResponse(req->getVersion() , req->isClose() || !m_iskeepalive)); 
        rsp->setBody("wyz httpserver");

        WYZ_LOG_INFO(g_logger) << "requst:\n" << *req;
        WYZ_LOG_INFO(g_logger) << "response:\n" << *rsp;

        session->sendResponse(rsp);
        if(!m_iskeepalive || req->isClose()){
            break;
        }

    }while (m_iskeepalive);
    session->close();
}


}
}