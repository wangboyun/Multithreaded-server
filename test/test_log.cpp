/*
 * @Description: 
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-09-22 19:39:25
 */

#include "../src/log.h"
#include "../src/util.h"
#include "../src/thread.h"
#include <iostream>

int main(){
    wyz::Logger::ptr logger(new wyz::Logger);
    wyz::StdoutLogAppender::ptr logstdout(new wyz::StdoutLogAppender);
    wyz::FileLogAppender::ptr logfile(new wyz::FileLogAppender("/home/wyz/workspace/wyz/logtest.txt"));
  
    logger->addAppender(logstdout);
    wyz::LogEvent::ptr event(new wyz::LogEvent(logger , wyz::LogLevel::DEBUG,__FILE__, __LINE__,  wyz::GetThreadId(), wyz::GetFiberId(), time(0), wyz::Thread::GetName()));
    event->getSstream() << "hello sylar log";
    logger->log(wyz::LogLevel::DEBUG, event);
    logger->addAppender(logfile);
    WYZ_LOG_INFO(logger)<< "test ";
    WYZ_LOG_ERROR(logger)<< "test error ";

    WYZ_LOG_FMT_INFO(logger, "test famt %s"," qr");
    WYZ_LOG_FMT_ERROR(logger, "test famt error %s","getthread fail");
    

    auto log = wyz::LoggerMgr::GetInstance()->getLogger("xx");
    WYZ_LOG_INFO(log) << "hello wyz";
    return 0;
}