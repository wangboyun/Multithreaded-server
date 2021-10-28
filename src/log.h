/*
 * @Description: 日志头文件
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-01 11:31:18
 */

#ifndef __WYZ_LOG_H__
#define __WYZ_LOG_H__


#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include "singleton.h"
#include "util.h"
#include "thread.h"

/**
 * @brief 使用流式方式将日志级别level的日志写入到logger
 */
#define WYZ_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        wyz::LogEventWrap(wyz::LogEvent::ptr(new wyz::LogEvent(logger, level, __FILE__, __LINE__, wyz::GetThreadId(),wyz::GetFiberId(), time(0), wyz::Thread::GetName()))).getSS()

/**
 * @brief 使用流式方式将日志级别debug的日志写入到logger
 */
#define WYZ_LOG_DEBUG(logger) WYZ_LOG_LEVEL(logger, wyz::LogLevel::DEBUG)

/**
 * @brief 使用流式方式将日志级别info的日志写入到logger
 */
#define WYZ_LOG_INFO(logger) WYZ_LOG_LEVEL(logger, wyz::LogLevel::INFO)

/**
 * @brief 使用流式方式将日志级别warn的日志写入到logger
 */
#define WYZ_LOG_WARN(logger) WYZ_LOG_LEVEL(logger, wyz::LogLevel::WARN)

/**
 * @brief 使用流式方式将日志级别error的日志写入到logger
 */
#define WYZ_LOG_ERROR(logger) WYZ_LOG_LEVEL(logger, wyz::LogLevel::ERROR)

/**
 * @brief 使用流式方式将日志级别fatal的日志写入到logger
 */
#define WYZ_LOG_FATAL(logger) WYZ_LOG_LEVEL(logger, wyz::LogLevel::FATAL)

/**
 * @brief 使用格式化方式将日志级别level的日志写入到logger
 */
#define WYZ_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        wyz::LogEventWrap(wyz::LogEvent::ptr(new wyz::LogEvent(logger, level, __FILE__, __LINE__,  wyz::GetThreadId(),wyz::GetFiberId(), time(0) , wyz::Thread::GetName()))).getEvent()->format(fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别debug的日志写入到logger
 */
#define WYZ_LOG_FMT_DEBUG(logger, fmt, ...) WYZ_LOG_FMT_LEVEL(logger, wyz::LogLevel::DEBUG, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别info的日志写入到logger
 */
#define WYZ_LOG_FMT_INFO(logger, fmt, ...)  WYZ_LOG_FMT_LEVEL(logger, wyz::LogLevel::INFO, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别warn的日志写入到logger
 */
#define WYZ_LOG_FMT_WARN(logger, fmt, ...)  WYZ_LOG_FMT_LEVEL(logger, wyz::LogLevel::WARN, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别error的日志写入到logger
 */
#define WYZ_LOG_FMT_ERROR(logger, fmt, ...) WYZ_LOG_FMT_LEVEL(logger, wyz::LogLevel::ERROR, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别fatal的日志写入到logger
 */
#define WYZ_LOG_FMT_FATAL(logger, fmt, ...) WYZ_LOG_FMT_LEVEL(logger, wyz::LogLevel::FATAL, fmt, __VA_ARGS__)

/**
 * @brief 获取主日志器
 */
#define WYZ_LOG_ROOT() wyz::LoggerMgr::GetInstance()->getRoot()

/**
 * @brief 获取name的日志器
 */
#define WYZ_LOG_NAME(name) wyz::LoggerMgr::GetInstance()->getLogger(name)

namespace wyz {

class Logger;
class LoggerManager;

class LogLevel{
public:
    /* 日志级别 */ 
    enum Level{
        UNKNOW,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL
    };
    /* 将日志级别 enum 转换为字符串 string*/
    static const std::string toString(const LogLevel::Level level);

    /* 将字符串 string 转换为将日志级别 enum*/
    static const LogLevel::Level formString(const std::string& str);
};  // 日志的等级类


class LogEvent{
public:
    using ptr = std::shared_ptr<LogEvent>;
    /**
     * @brief 构造函数
     * @param[in] logger 日志器
     * @param[in] level 日志级别
     * @param[in] file 文件名
     * @param[in] line 文件行号
     * @param[in] elapse 程序启动依赖的耗时(毫秒)
     * @param[in] thread_id 线程id
     * @param[in] fiber_id 协程id
     * @param[in] time 日志事件(秒)
     * @param[in] thread_name 线程名称
     */
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level ,const std::string& file , uint32_t line,uint32_t thread_id ,uint32_t fiber_id,uint32_t time , const std::string& thread_name);

    /* 一系列的get/set 方法 */
    // get 日志器
    inline std::shared_ptr<Logger> getLogger() {return m_logger;}

    // get/set 优先级
    inline const LogLevel::Level getLevel()const  { return m_level;}
    inline void setLevel(const LogLevel::Level level) { m_level = level;}

    // get文件名
    inline const std::string& getFileName()const { return m_file;};
    
    // get 行号
    inline const uint32_t getLine()const    {return m_line;}

    // get 线程号
    inline const uint32_t getThreadId()const {return m_threadId;}

    // get 协程号
    inline const uint32_t getFribeId()const  {return m_fiberId;}

    // get 时间
    inline const uint64_t getTime()const    {return m_time;}

    // get文件流
    inline std::stringstream& getSstream()   {return m_ss;};

    // get 内容
    inline std::string  getContent()const   {return m_ss.str();}

    //get 线程名称
    inline const std::string& getThread_name()const {return m_thread_name;}

    /**
     * @brief 格式化写入日志内容
    */
    void format(const char* fmt, ...);

    /**
     * @brief 格式化写入日志内容
     */
    void format(const char* fmt, va_list al);

private:
    std::shared_ptr<Logger> m_logger;   // 日志器
    LogLevel::Level m_level ;           // 事件优先级
    const std::string m_file;           // 文件名
    uint32_t m_line;                    // 行号
    uint32_t m_threadId;                // 线程号
    uint32_t m_fiberId;                 // 协程号
    uint64_t m_time;                    // 时间
    std::stringstream m_ss;             // 输出文件流
    std::string m_thread_name;          // 线程名称

};  // 日志事件类

class LogEventWrap{
public:
    LogEventWrap(LogEvent::ptr e);
    ~LogEventWrap();    // 在析构函数调用打印函数
    /**
     * @brief 获取日志事件
     */
    inline LogEvent::ptr getEvent() const { return m_event;}

    /**
     * @brief 获取日志内容流
     */
    std::stringstream& getSS();

private:
    LogEvent::ptr m_event;

};  // 日志事件包装器

class LogFormatter{
public:
    using ptr = std::shared_ptr<LogFormatter>;

    LogFormatter(const std::string& pattern);
    /**
     * @brief 返回格式化日志文本
     * @param[in] logger 日志器
     * @param[in] level 日志级别
     * @param[in] event 日志事件
     */
    std::string format( std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event);
    std::ostream& format(std::ostream& ofs, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event);

    void init();        // 解析pattern 各个格式
    inline bool IsError() const     {return m_error;}

    /*get 方法*/
    inline const std::string getPattern()const {return m_pattern;}

public:
    class FormatItem{
    public:
        using ptr = std::shared_ptr<FormatItem>;

        virtual ~FormatItem(){};

        /**
         * @brief 格式化日志到流
         * @param[in, out] os 日志输出流
         * @param[in] logger 日志器
         * @param[in] level 日志等级
         * @param[in] event 日志事件
         */
        virtual void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) = 0;
    };

private:
    std::string m_pattern;          // 给定的输出格式
    bool m_error = false;           // 解析的格式是否有错误
    std::vector<FormatItem::ptr> m_items;  // 解析出来的各个对应的格式

};  // 日志格式类


class LogAppender{
friend class Logger;
public:
    using ptr = std::shared_ptr<LogAppender>;
    using MutexType = SpinLock;

    virtual ~LogAppender(){}
    /**
     * @brief 写入日志
     * @param[in] logger 日志器
     * @param[in] level 日志级别
     * @param[in] event 日志事件
     */
    virtual void log(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) = 0;

    /* 一系列 get/set 方法*/
    //get/set  日志级别
    inline const LogLevel::Level getLevel()const  {return m_level;};
    inline void setLevel(LogLevel::Level level)   {m_level = level;}
    
    // get/set输出格式
    const LogFormatter::ptr getFormatter();
    void setFormatter(LogFormatter::ptr format);
    
    /**
     * @brief 将日志输出目标的配置转成YAML String
     */
    virtual std::string toYamlString() = 0;

protected:
    LogLevel::Level m_level = LogLevel::UNKNOW;     /* 日志输出地的级别 */
    LogFormatter::ptr m_formatter;                  /* 日志输出格式*/
    MutexType m_mutex;                              /* 平凡的文件写入*/
    bool m_hasFormatter = false;                    /* 判断不同的输出目的地有无格式*/
};  // 日志打印的目的地类

class StdoutLogAppender : public LogAppender{
public:
    using ptr = std::shared_ptr<StdoutLogAppender>;
    StdoutLogAppender(){}
    void log(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override;
    /**
     * @brief 将日志输出目标的配置转成YAML String
     */
    std::string toYamlString()override;
};  // 标准输出类 公有继承LogAppenden

class FileLogAppender : public LogAppender{
public:
    using ptr = std::shared_ptr<FileLogAppender>;
    FileLogAppender(const std::string& filename);
    void log(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override;
     /**
     * @brief 重新打开日志文件
     */
    void reopen();
    /**
     * @brief 将日志输出目标的配置转成YAML String
     */
    std::string toYamlString()override;
private:
    std::string m_filename ;
    /// 文件流
    std::ofstream m_filestream;
};  // 文件输出类 公有继承LogAppenden

class Logger : public std::enable_shared_from_this<Logger>{
friend class LoggerManager;
public:
    using ptr = std::shared_ptr<Logger>;
    using  MutexType = SpinLock;

    Logger(const std::string& name = "root");
    /**
     * @brief 写日志
     * @param[in] level 日志级别
     * @param[in] event 日志事件
     */
    void log(LogLevel::Level level, LogEvent::ptr event);

    /**
     * @brief 写debug级别日志
     * @param[in] event 日志事件
     */
    void debug(LogEvent::ptr event);

    /**
     * @brief 写info级别日志
     * @param[in] event 日志事件
     */
    void info(LogEvent::ptr event);

    /**
     * @brief 写warn级别日志
     * @param[in] event 日志事件
     */
    void warn(LogEvent::ptr event);

    /**
     * @brief 写error级别日志
     * @param[in] event 日志事件
     */
    void error(LogEvent::ptr event);

    /**
     * @brief 写fatal级别日志
     * @param[in] event 日志事件
     */
    void fatal(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    void clearAppenders();

    /* 一系列get/set 方法 */
    // get 日志名称名称
    inline const std::string& getName()const {return m_name;}

    // get/set 日志级别
    inline const LogLevel::Level getLevel()const {return m_level;}
    inline void setLevel(LogLevel::Level level) {m_level = level;}
    
    // get/set 日志格式
    const LogFormatter::ptr getFormatter();
    void setFormatter(LogFormatter::ptr format); 
    void setFormatter(const std::string& val);
    /**
     * @brief 将日志器的配置转成YAML String
     */
    std::string toYamlString();

private:
    std::string m_name;                      // 日志名称
    LogLevel::Level m_level;                 // 日志级别
    std::list<LogAppender::ptr> m_appenders; // 日志目标集合
    LogFormatter::ptr m_formatter;           // 日志格式
    /* 主日志器 */
    Logger::ptr m_root;
    MutexType m_mutex;                      // mutex 互斥量

};  // 日志器类

class LoggerManager{
public:
    using MutexType = SpinLock;

    LoggerManager();
    /* 获取日志器 */
    Logger::ptr getLogger(const std::string& name);

    /* 获取主日志器 */
    Logger::ptr getRoot()const {return m_root;}

    /**
     * @brief 将所有的日志器配置转成YAML String
     */
    std::string toYamlString();

private:
    /* 日志管理容器 */
    std::map<std::string, Logger::ptr> m_loggers;

    /* 主日志器 */
    Logger::ptr m_root;

    MutexType m_mutex;
};  // 日志管理器

/// 日志器管理类单例模式
using LoggerMgr =  wyz::Singleton<LoggerManager>;

}

#endif