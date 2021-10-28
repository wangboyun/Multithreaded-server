/*
 * @Description: 
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-01 11:31:25
 */

#include <cctype>
#include <cstddef>
#include <functional>
#include <pthread.h>
#include <time.h>
#include <sstream>
#include <string>
#include <tuple>
#include <iostream>
#include <stdarg.h>
#include <vector>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include "log.h"
#include "util.h"
#include "config.h"


namespace wyz {

/* 将日志级别 enum 转换为字符串 string*/
const std::string LogLevel::toString(const LogLevel::Level level){
    switch (level) {
        case LogLevel::DEBUG :
            return "DEBUG";
        case LogLevel::INFO :
            return "INFO";
        case LogLevel::WARN :
            return "WARN";
        case LogLevel::ERROR :
            return "ERROR";
        case LogLevel::FATAL :
            return "FATAL";
        default:
            return "UNKNOW";
    }
}

    /* 将字符串 string 转换为将日志级别 enum*/
const LogLevel::Level LogLevel::formString(const std::string& str){
    
    if(str == "DEBUG" || str == "debug") 
        return LogLevel::DEBUG;
    else if(str == "INFO" || str == "info")
        return LogLevel::INFO;
    else if(str == "WARN" || str == "warn")
        return LogLevel::WARN;
    else if(str == "ERROR" || str == "error")
        return LogLevel::ERROR;
    else if(str == "FATAL" || str == "fatal")
        return LogLevel::FATAL;
    else
        return LogLevel::UNKNOW;
}

LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level ,const std::string& file , uint32_t line,uint32_t thread_id ,uint32_t fiber_id,uint32_t time , const std::string& thread_name )
    : m_logger(logger)
    , m_level(level)
    , m_file(file)
    , m_line(line)
    , m_threadId(thread_id)
    , m_fiberId(fiber_id)
    , m_time(time)
    , m_thread_name(thread_name){
}

/**
* @brief 格式化写入日志内容
*/
void LogEvent::format(const char* fmt, ...){
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);
}

/**
* @brief 格式化写入日志内容
*/
void LogEvent::format(const char* fmt, va_list al) {
    char* buf = nullptr;
    int len = vasprintf(&buf, fmt, al);
    if(len != -1) {
        m_ss << std::string(buf, len);
        free(buf);
    }
}

LogEventWrap::LogEventWrap(LogEvent::ptr e)
    : m_event(e){
}
// 在析构函数调用打印函数
LogEventWrap::~LogEventWrap(){
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}
/**
* @brief 获取日志内容流
*/
std::stringstream& LogEventWrap::getSS() {
    return m_event->getSstream();
}


LogFormatter::LogFormatter(const std::string& pattern)
    : m_pattern(pattern){
    init();
}
    /**
     * @brief 返回格式化日志文本
     * @param[in] logger 日志器
     * @param[in] level 日志级别
     * @param[in] event 日志事件
     */
std::string LogFormatter::format( std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event){
    std::stringstream ss;
    for(auto it : m_items){
        it->format(ss, logger, level, event);
    }
    return ss.str();
}

std::ostream& LogFormatter::format(std::ostream& ofs, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event){
    for(auto it : m_items){
        it->format(ofs, logger, level, event);
    }
    return ofs;
}

class MessageFormatItem : public LogFormatter::FormatItem{
public:
    MessageFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        os << event->getContent();
    }
};  // 消息格式对应的类     %m

class LevelFormatItem : public LogFormatter::FormatItem{
public:
    LevelFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        os << LogLevel::toString(level);
    }
};  // 日志级别格式对应的类 %p

class NameFormatItem : public LogFormatter::FormatItem{
public:
    NameFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        os << event->getLogger()->getName();
    }
};  // 日志名称格式对应的类 %c

class ThreadIdFormatItem : public LogFormatter::FormatItem{
public:
    ThreadIdFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        os << event->getThreadId();
    }
};  // 线程id格式对应的类 %t

class NextLineFormatItem : public LogFormatter::FormatItem{
public:
    NextLineFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        os << std::endl;
    }
};  //换行格式对应的类 %n

class TimeFormatItem : public LogFormatter::FormatItem{
public:
    TimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S") 
    : m_format(format){
        if(m_format.empty())
            m_format = "%Y-%m-%d %H:%M:%S";
    }
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        char buf[64];
        time_t time = event->getTime();
        struct tm* temp;
        temp = localtime(&time);
        strftime(buf, sizeof(buf), m_format.c_str(), temp);
        os << buf;
    }
private:
    std::string m_format;
};  // 时间格式对应的类 %d

class FilenameFormatItem : public LogFormatter::FormatItem{
public:
    FilenameFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        os << event->getFileName();
    }
};  //  文件名称格式对应的类 %f

class LineFormatItem : public LogFormatter::FormatItem{
public:
    LineFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        os << event->getLine();
    }
};  //  行号格式对应的类 %l

class TabFormatItem : public LogFormatter::FormatItem{
public:
    TabFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        os << "\t";
    }
};  // 制表符格式对应的类 %T

class FriberIdFormatItem : public LogFormatter::FormatItem{
public:
    FriberIdFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        os << event->getFribeId();
    }
};  // 协程id格式对应的类 %F

class ThreadNameFormatItem : public LogFormatter::FormatItem{
public:
    ThreadNameFormatItem(const std::string& str = ""){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        os << event->getThread_name();
    }
};      // 线程名称 %N

class StringFormatItem : public LogFormatter::FormatItem{
public:
    StringFormatItem(const std::string& str) : nstr(str){}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        os << nstr;
    }
private:
    const std::string nstr;
};


/*
*  %m 消息
*  %p 日志级别
*  %r 累计毫秒数
*  %c 日志名称
*  %t 线程id
*  %n 换行
*  %d 时间
*  %f 文件名
*  %l 行号
*  %T 制表符
*  %F 协程id
*  %N 线程名称
*/
// 格式如下 %d{%H} %f %%
void LogFormatter::init(){
    std::vector<std::tuple<std::string, std::string, int> > vec;
    std::string nstr;
    for(size_t i = 0; i < m_pattern.size(); ++i) {
        if(m_pattern[i] != '%') {
            nstr.append(1, m_pattern[i]);
            continue;
        }

        if((i + 1) < m_pattern.size()) {
            if(m_pattern[i + 1] == '%') {
                nstr.append(1, '%');
                continue;
            }
        }

        size_t n = i + 1;
        int fmt_status = 0;
        size_t fmt_begin = 0;

        std::string str;
        std::string fmt;
        while(n < m_pattern.size()) {
            if(!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{'
                    && m_pattern[n] != '}')) {
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }
            if(fmt_status == 0) {
                if(m_pattern[n] == '{') {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    //std::cout << "*" << str << std::endl;
                    fmt_status = 1; //解析格式
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            } else if(fmt_status == 1) {
                if(m_pattern[n] == '}') {
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    //std::cout << "#" << fmt << std::endl;
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if(n == m_pattern.size()) {
                if(str.empty()) {
                    str = m_pattern.substr(i + 1);
                }
            }
        }

        if(fmt_status == 0) {
            if(!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        } else if(fmt_status == 1) {
            std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    }

    if(!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }
    static std::map<std::string, std::function<FormatItem::ptr(const std::string&str)>> s_format_items = {
        {"m" , [](const std::string&str){return FormatItem::ptr(new MessageFormatItem(str));}},   //m:消息
        {"p" , [](const std::string&str){return FormatItem::ptr(new LevelFormatItem(str));}},    //p:日志级别
        {"c" , [](const std::string&str){return FormatItem::ptr(new NameFormatItem(str));}},    //c 日志名称
        {"t" , [](const std::string&str){return FormatItem::ptr(new ThreadIdFormatItem(str));}},  //t 线程id
        {"n" , [](const std::string&str){return FormatItem::ptr(new NextLineFormatItem(str));}},  //n 换行
        {"d" , [](const std::string&str){return FormatItem::ptr(new TimeFormatItem(str));}},      //d 时间
        {"f" , [](const std::string&str){return FormatItem::ptr(new FilenameFormatItem(str));}},  //f 文件名
        {"l" , [](const std::string&str){return FormatItem::ptr(new LineFormatItem(str));}},      //l 行号
        {"T" , [](const std::string&str){return FormatItem::ptr(new TabFormatItem(str));}},       //T 制表符
        {"F" , [](const std::string&str){return FormatItem::ptr(new FriberIdFormatItem(str));}},  //F 协程id
        {"N" , [](const std::string&str){return FormatItem::ptr(new ThreadNameFormatItem(str));}} //N 线程名称
    };
    // 现在将解析出来的格式与map中的函数对应
    for(auto i : vec) {
        if(std::get<2>(i) == 0) {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        } else {
            auto it = s_format_items.find(std::get<0>(i));
            if(it == s_format_items.end()) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                m_error = true;
            } else {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }

        // std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
    }
    // std::cout << m_items.size() << std::endl;
}       // 解析pattern 各个格式

const LogFormatter::ptr LogAppender::getFormatter(){
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}

void LogAppender::setFormatter(LogFormatter::ptr format){
    MutexType::Lock lock(m_mutex); 
    m_formatter = format;
    if(m_formatter){
        m_hasFormatter = true;
    }else {
        m_hasFormatter = false;
    }
}


void StdoutLogAppender::log(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event){
    if(level >= m_level){
        MutexType::Lock lock(m_mutex);             // 构造互斥量对象   
        m_formatter->format(std::cout, logger,level,event);
    }
}

std::string StdoutLogAppender::toYamlString(){
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    if(m_level != LogLevel::UNKNOW)
        node["level"] = LogLevel::toString(m_level);
    if(m_hasFormatter && m_formatter){
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}


FileLogAppender::FileLogAppender(const std::string& filename)
    : m_filename(filename){
    reopen();
}

void FileLogAppender::log(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event){
    if(level >= m_level){
        MutexType::Lock lock(m_mutex);
        reopen();
        m_formatter->format(m_filestream , logger,level,event);
    }
}
/**
* @brief 重新打开日志文件
*/
void FileLogAppender::reopen(){
    if(m_filestream.is_open()) {
        m_filestream.close();
    }
    //以追加方式
    m_filestream.open(m_filename,std::ios_base::app); 
}

std::string FileLogAppender::toYamlString(){
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    if(m_level != LogLevel::UNKNOW)
        node["level"] = LogLevel::toString(m_level);
    if(m_hasFormatter && m_formatter){
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}


Logger::Logger(const std::string& name) 
    : m_name(name)
    , m_level(LogLevel::DEBUG){
    m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}
/**
* @brief 写日志
* @param[in] level 日志级别
* @param[in] event 日志事件
*/
void Logger::log(LogLevel::Level level, LogEvent::ptr event){
    if(level >= m_level){
        auto self = shared_from_this();
        if(!m_appenders.empty()){
            for(auto i : m_appenders){
                i->log(self, level, event);
            }
        }else {
            m_root->log(level, event);
        }
    }
}

/**
* @brief 写debug级别日志
* @param[in] event 日志事件
*/
void Logger::debug(LogEvent::ptr event){
    log(LogLevel::DEBUG, event);
}

/**
* @brief 写info级别日志
* @param[in] event 日志事件
*/
void Logger::info(LogEvent::ptr event){
    log(LogLevel::INFO, event);
}

/**
* @brief 写warn级别日志
* @param[in] event 日志事件
*/
void Logger::warn(LogEvent::ptr event){
    log(LogLevel::WARN, event);
}

/**
* @brief 写error级别日志
* @param[in] event 日志事件
*/
void Logger::error(LogEvent::ptr event){
    log(LogLevel::ERROR, event);
}

/**
* @brief 写fatal级别日志
* @param[in] event 日志事件
*/
void Logger::fatal(LogEvent::ptr event){
    log(LogLevel::FATAL, event);
}

void Logger::addAppender(LogAppender::ptr appender){
    MutexType::Lock lock(m_mutex);
    if(!appender->getFormatter()) {
        MutexType::Lock m_lock(appender->m_mutex);
        appender->m_formatter = m_formatter;
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender){
    MutexType::Lock lock(m_mutex);
    for(auto it = m_appenders.begin(); it != m_appenders.end() ; ++ it){
        if(*it == appender){
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::clearAppenders(){
    MutexType::Lock lock(m_mutex);
    m_appenders.clear();
}

std::string Logger::toYamlString(){
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["name"] = m_name;
    if(m_level != LogLevel::UNKNOW){
        node["level"] = LogLevel::toString(m_level);
    }
    if(m_formatter){
        node["formatter"] = m_formatter->getPattern();
    }
    for(auto it : m_appenders){
        node["appenders"].push_back(YAML::Load(it->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

const LogFormatter::ptr Logger::getFormatter(){
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}

void Logger::setFormatter(LogFormatter::ptr format){
    MutexType::Lock lock(m_mutex);
    m_formatter = format;
    for(auto it : m_appenders){
        if(!it->m_hasFormatter){
            it->m_formatter = m_formatter;
        }
    }
}

void Logger::setFormatter(const std::string& val){
    LogFormatter::ptr new_val = LogFormatter::ptr(new LogFormatter(val));
    if(new_val->IsError()){
        std::cout << "Logger setFormatter name=" << m_name
                  << " value=" << val << " invalid formatter"
                  << std::endl;
        return;
    }
    setFormatter(new_val);
}

LoggerManager::LoggerManager(){
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
    m_loggers[m_root->m_name] = m_root;
}

/* 获取日志器 */
Logger::ptr LoggerManager::getLogger(const std::string& name){
    MutexType::Lock lock(m_mutex);
    auto it = m_loggers.find(name);
    if(it != m_loggers.end()){
        return it->second;
    }
    Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;
    m_loggers[name] = logger;
    return logger;
}

/* 对应的是 log.yaml 文件中的appenders 下的数据 */
struct LogAppenderDefine{
    int type = 0; // 1 -- file  2 -- stdout
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string file;
    std::string formatter;
    bool operator== (const LogAppenderDefine& oth) const{
        return type == oth.type
            && level == oth.level
            && file == oth.file
            && formatter == oth.formatter;
    }
};

struct LogDefine{
    std::string name;
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::vector< LogAppenderDefine > appenders;
    bool operator==(const LogDefine& oth) const {
        return name == oth.name
            && level == oth.level
            && formatter == oth.formatter;
    }

    bool operator<(const LogDefine& oth) const {
        return name < oth.name;
    }

    bool isValid() const {
        return !name.empty();
    }
};

template<>
class LexicalCast<std::string, LogDefine>{
public:
    LogDefine operator() (const std::string& str){
        YAML::Node n = YAML::Load(str);
        LogDefine ld;
        if(!n["name"].IsDefined()){
            std::cout << "log config error: name is null, " << n
                      << std::endl;
            throw std::logic_error("log config name is null");
        }
        ld.name = n["name"].as<std::string>();
        
        if(!n["level"].IsDefined()){
            ld.level = LogLevel::formString("");
        }
        ld.level = LogLevel::formString(n["level"].as<std::string>());

        if(n["formatter"].IsDefined())
            ld.formatter = n["formatter"].as<std::string>();
        if(n["appenders"].IsDefined()){
            for(size_t x = 0 ; x < n["appenders"].size(); ++x){
                auto a = n["appenders"][x];
                if(!a["type"].IsDefined()){
                    std::cout << "log config error: appender type is null, " << a
                              << std::endl;
                    continue;
                } 
                std::string type = a["type"].as<std::string>();
                LogAppenderDefine lad;
                if(type == "FileLogAppender"){
                    lad.type = 1;
                    if(!a["file"].IsDefined()){
                        std::cout << "log config error: fileappender file is null, " << a
                              << std::endl;
                        continue;
                    }
                    lad.file = a["file"].as<std::string>();
                    if(a["formatter"].IsDefined()){
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                    
                }else if(type == "StdoutLogAppender"){
                    lad.type = 2;
                    if(a["formatter"].IsDefined()) {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                    
                }else {
                    std::cout << "log config error: appender type is invalid, " << a
                              << std::endl;
                    continue;
                }
                ld.appenders.emplace_back(lad);
            }
        }
        return ld;
    }
};  // 从string 类型转换为 LogDefine 类型


template<>
class LexicalCast<LogDefine, std::string>{
public:
    std::string operator() (const LogDefine& i){
        YAML::Node n;
        n["name"] = i.name;
        if(i.level != LogLevel::UNKNOW)
            n["level"] = LogLevel::toString(i.level);
        if(!i.formatter.empty()){
            n["formatter"] = i.formatter;
        }
        for(auto a : i.appenders){
            YAML::Node na;
            if(a.type == 1){
                na["type"] = "FileLogAppender";
                na["file"] = a.file;
            }else if (a.type == 2) {
                na["type"] = "StdoutLogAppender";
            }
            if(a.level != LogLevel::UNKNOW)
                na["level"] = LogLevel::toString(a.level);
            if(!a.formatter.empty()){
                na["formatter"] = a.formatter;
            }
            n["appenders"].push_back(na);
        }
        std::stringstream ss;
        ss << n;
        return ss.str();
    }
};  // 从 LogDefine类型转换为 string 类型

wyz::ConfigVar<std::set<LogDefine> >::ptr g_log_defines =
    wyz::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

struct LogIniter {
    LogIniter() {
        g_log_defines->addListener([](const std::set<LogDefine>& old_value,
                    const std::set<LogDefine>& new_value){
            WYZ_LOG_INFO(WYZ_LOG_ROOT()) << "on_logger_conf_changed";
            for(auto i : new_value) {
                auto it = old_value.find(i);
                wyz::Logger::ptr logger;
                if(it == old_value.end()) {
                    //新增logger
                    logger = WYZ_LOG_NAME(i.name);
                } else {
                    if(!(i == *it)) {
                        //修改的logger
                        logger = WYZ_LOG_NAME(i.name);
                    } else {
                        continue;
                    }
                }
                
                logger->setLevel(i.level);
                // std::cout << "** " << i.name << " level=" << i.level
                // << "  " << logger << std::endl;
                if(!i.formatter.empty()) {
                    logger->setFormatter(i.formatter);
                }

                logger->clearAppenders();
                for(auto a : i.appenders) {
                    wyz::LogAppender::ptr ap;
                    if(a.type == 1) {
                        ap.reset(new FileLogAppender(a.file));
                    } else if(a.type == 2) {
                        ap.reset(new StdoutLogAppender);
                    }
                    if(a.level != LogLevel::UNKNOW)
                        ap->setLevel(a.level);
                    if(!a.formatter.empty()) {
                        LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                        if(!fmt->IsError()) {
                            ap->setFormatter(fmt);
                        } else {
                            std::cout << "log.name=" << i.name << " appender type=" << a.type
                                      << " formatter=" << a.formatter << " is invalid" << std::endl;
                        }
                    }
                    logger->addAppender(ap);
                }
            }

            for(auto& i : old_value) {
                auto it = new_value.find(i);
                if(it == new_value.end()) {
                    //删除logger
                    auto logger = WYZ_LOG_NAME(i.name);
                    logger->setLevel((LogLevel::Level)0);
                    logger->clearAppenders();
                }
            }
        });
    }
};

static LogIniter __log_init;


std::string LoggerManager::toYamlString(){
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    for(auto it : m_loggers){
        node.push_back(YAML::Load(it.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}


}