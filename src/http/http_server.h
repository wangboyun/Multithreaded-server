/**
 * @file http_server.h
 * @brief http 服务器端封装
 * @author wyz (501826086@qq.com)
 * @version 1.0
 * @date 2021-11-20
 * 
 * @copyright Copyright (c) 2021  wyz
 * 
 */

#ifndef __WYZ_HTTP_SERVER_H__
#define __WYZ_HTTP_SERVER_H__

#include "../tcpserver.h"
#include <memory>

namespace wyz {
namespace http {

class HttpServer : public TCPServer{
public:
    using ptr = std::shared_ptr<HttpServer>;
    HttpServer(bool keepalive = false , IOManager* worker = IOManager::GetThis(), IOManager* acceptworker = IOManager::GetThis());

protected:
    virtual void handleClient(Socket::ptr client) override;

private:
    bool m_iskeepalive;
};

}
}




#endif