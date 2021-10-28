/*
 * @Description: 配置模块
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-04 11:16:53
 */
#ifndef _WYZ_CONFIG_H__
#define _WYZ_CONFIG_H__

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include "util.h"
#include "log.h"
#include "mutex.h"

namespace wyz {

class ConfigVarBase{
public:
    using ptr = std::shared_ptr<ConfigVarBase>;
    /**
     * @brief 构造函数
     * @param[in] name 配置参数名称[0-9a-z_.]
     * @param[in] description 配置参数描述
     */
    ConfigVarBase(const std::string& name , const std::string& description)
    : m_name(name)
    , m_descripiton(description){
        std::transform(m_name.begin(),m_name.end(),m_name.begin(), ::tolower);
    }

    virtual ~ConfigVarBase() {}
    /* get 函数 */
    // get 名字 描述
    inline const std::string getName()const  {return m_name;}
    inline const std::string getDescrip()const  {return m_descripiton;}
    /**
     * @brief 转成字符串
     */
    virtual std::string toString() = 0;

    /**
     * @brief 从字符串初始化值
     */
    virtual bool fromString(const std::string& val) = 0;

     /**
     * @brief 返回配置参数值的类型名称
     */
    virtual std::string getTypeName() const = 0;
protected:
    std::string m_name;
    std::string m_descripiton;
};  // 配置变量的基类


// 转换从F类型 转变为T类型
template<class F , class T>
class LexicalCast{
public:
    /**
     * @brief 类型转换
     * @param[in] val 源类型值
     * @return 返回val转换后的目标类型
     * @exception 当类型不可转换时抛出异常
     */
    T operator() (const F& val){
        return boost::lexical_cast<T>(val);
    }
};

// 支持一系列的stl 模板容器 vector list set unordered_set map unordered_map
template<class T>
class LexicalCast<std::string, std::vector<T>>{
public:
    std::vector<T> operator() (const std::string& str){
        YAML::Node node = YAML::Load(str);
        typename std::vector<T> vec;
        std::stringstream ss;
        for(size_t i = 0 ; i < node.size() ; ++i){
            ss.str("");
            ss << node[i];
            vec.emplace_back(LexicalCast<std::string, T>() (ss.str()));
        }
        return vec;
    }
};  // 从string转变为vector<T> 类型

template<class T>
class LexicalCast<std::vector<T>, std::string>{
public: 
    std::string operator() (std::vector<T> vec){
        std::stringstream ss;
        YAML::Node node;
        for(auto it : vec){
            node.push_back(YAML::Load(LexicalCast<T,std::string>()(it)));
        }
        ss << node;
        return ss.str();
    }
};  // 从 vector<T> 转换为 std::string类型

template<class T>
class LexicalCast<std::string, std::list<T>>{
public:
    std::list<T> operator() (const std::string& str){
        YAML::Node node = YAML::Load(str);
        typename std::list<T> vec;
        std::stringstream ss;
        for(size_t i = 0 ; i < node.size() ; ++i){
            ss.str("");
            ss << node[i];
            vec.emplace_back(LexicalCast<std::string, T>() (ss.str()));
        }
        return vec;
    }
};  // 从string转变为list<T> 类型

template<class T>
class LexicalCast<std::list<T>, std::string>{
public: 
    std::string operator() (std::list<T> vec){
        std::stringstream ss;
        YAML::Node node;
        for(auto it : vec){
            node.push_back(YAML::Load(LexicalCast<T,std::string>()(it)));
        }
        ss << node;
        return ss.str();
    }
};  // 从 list<T> 转换为 std::string类型

template<class T>
class LexicalCast<std::string, std::set<T>>{
public:
    std::set<T> operator() (const std::string& str){
        YAML::Node node = YAML::Load(str);
        typename std::set<T> vec;
        std::stringstream ss;
        for(size_t i = 0 ; i < node.size() ; ++i){
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>() (ss.str()));
        }
        return vec;
    }
};  // 从string转变为set<T> 类型

template<class T>
class LexicalCast<std::set<T>, std::string>{
public: 
    std::string operator() (std::set<T> vec){
        std::stringstream ss;
        YAML::Node node;
        for(auto it : vec){
            node.push_back(YAML::Load(LexicalCast<T,std::string>()(it)));
        }
        ss << node;
        return ss.str();
    }
};  // 从 list<T> 转换为 std::string类型

template<class T>
class LexicalCast<std::string, std::unordered_set<T>>{
public:
    std::unordered_set<T> operator() (const std::string& str){
        YAML::Node node = YAML::Load(str);
        typename std::unordered_set<T> vec;
        std::stringstream ss;
        for(size_t i = 0 ; i < node.size() ; ++i){
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>() (ss.str()));
        }
        return vec;
    }
};  // 从string转变为unordered_set<T> 类型

template<class T>
class LexicalCast<std::unordered_set<T>, std::string>{
public: 
    std::string operator() (std::unordered_set<T> vec){
        std::stringstream ss;
        YAML::Node node;
        for(auto it : vec){
            node.push_back(YAML::Load(LexicalCast<T,std::string>()(it)));
        }
        ss << node;
        return ss.str();
    }
};  // 从 unordered_set<T> 转换为 std::string类型

template<class T>
class LexicalCast<std::string, std::map<std::string,T>>{
public:
    std::map<std::string,T> operator() (const std::string& str){
        YAML::Node node = YAML::Load(str);
        typename std::map<std::string,T> vec;
        std::stringstream ss;
        for(auto it = node.begin(); it != node.end(); ++it){
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>() (ss.str()))); 
        }
        return vec;
    }
};  // 从string转变为std::map<std::string,T> 类型

template<class T>
class LexicalCast<std::map<std::string,T>, std::string>{
public: 
    std::string operator() (std::map<std::string,T> vec){
        std::stringstream ss;
        YAML::Node node;
        for(auto it : vec){
            node[it.first] = (YAML::Load(LexicalCast<T,std::string>()(it.second)));
        }
        ss << node;
        return ss.str();
    }
};  // 从 std::map<std::string,T> 转换为 std::string类型

template<class T>
class LexicalCast<std::string, std::unordered_map<std::string,T>>{
public:
    std::unordered_map<std::string,T> operator() (const std::string& str){
        YAML::Node node = YAML::Load(str);
        typename std::unordered_map<std::string,T> vec;
        std::stringstream ss;
        for(auto it = node.begin(); it != node.end(); ++it){
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>() (ss.str())));
        }
        return vec;
    }
};  // 从string转变为std::unordered_map<std::string,T> 类型

template<class T>
class LexicalCast<std::unordered_map<std::string,T>, std::string>{
public: 
    std::string operator() (std::unordered_map<std::string,T> vec){
        std::stringstream ss;
        YAML::Node node;
        for(auto it : vec){
            node[it.first] = (YAML::Load(LexicalCast<T,std::string>()(it.second)));
        }
        ss << node;
        return ss.str();
    }
};  // 从 std::unordered_map<std::string,T> 转换为 std::string类型


/* 模板类 通过继承configVarbase 重构tosting ，fromstring 
将模板类转换为特定的数据类型 */
template<class T , class ToStr = LexicalCast<T, std::string>
, class FromStr = LexicalCast<std::string, T>>
class ConfigVar : public ConfigVarBase{
public:
    using ptr = std::shared_ptr<ConfigVar>;
    using on_change_cb = std::function<void (const T& old_val , const T& new_val)>;
    using RWMutexType = RWMutex;

    /**
     * @brief 通过参数名,参数值,描述构造ConfigVar
     * @param[in] name 参数名称有效字符为[0-9a-z_.]
     * @param[in] default_value 参数的默认值
     * @param[in] description 参数的描述
     */
    ConfigVar(const std::string& name , const std::string& description, const T& default_value )
    : ConfigVarBase(name,description)
    , m_val(default_value){
    }

    std::string toString()override{
        try {
            RWMutexType::WriteLock lock(m_mutex);
            return ToStr()(m_val);
        } catch (std::exception& e) {
            WYZ_LOG_ERROR(WYZ_LOG_ROOT()) << "ConfigVar::toString exception "
                << e.what() << " convert: " << getTypeName() << " to string"
                << " name=" << m_name;
        }
        return "";
    }

    bool fromString(const std::string& val)override {
        try {
            setValue(FromStr()(val));
            return true;
        } catch (std::exception& e) {
            WYZ_LOG_ERROR(WYZ_LOG_ROOT()) << "ConfigVar::fromString exception "
                << e.what() << " convert: " << getTypeName() << " from string"
                << " name=" << m_name << " - " << val;;
        }
        return false;
    }
    /* 一系列get/set 方法*/
    const T getValue()  {
        RWMutexType::ReadLock lock(m_mutex);
        return m_val;
    }

    void setValue(const T& val){
        {   // {} 作用是给锁类一个局部范围，便于调用析构解锁
            RWMutexType::ReadLock lock(m_mutex);
            if(val == m_val) {
                return;
            }
            for(auto& i : m_cbs) {
                i.second(m_val, val);
            }
        } 
        RWMutexType::WriteLock lock(m_mutex);
        m_val = val;
    }
    
    std::string getTypeName() const override { return typeid(T).name();}

    /**
     * @brief 添加变化回调函数
     * @return 返回该回调函数对应的唯一id,用于删除回调
     */
    uint64_t addListener(on_change_cb cb){
        static uint64_t s_fun_id = 0;
        RWMutexType::WriteLock lock(m_mutex);
        ++ s_fun_id;
        m_cbs[s_fun_id] = cb;
        return s_fun_id;
    }

    /**
     * @brief 删除回调函数
     * @param[in] key 回调函数的唯一id
     */
    void delListener(uint64_t key) {
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.erase(key);
    }

    /**
     * @brief 获取回调函数
     * @param[in] key 回调函数的唯一id
     * @return 如果存在返回对应的回调函数,否则返回nullptr
     */
    on_change_cb getListener(uint64_t key) {
        RWMutexType::ReadLock lock(m_mutex);
        auto it = m_cbs.find(key);
        if(it != m_cbs.end())
            return it.second;
        else
            return nullptr;
    }
    /**
     * @brief 清理所有的回调函数
     */
    void clearListener() {
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.clear();
    }

private:
    T m_val;
    //变更回调函数组, uint64_t key,要求唯一，一般可以用hash
    std::map<uint64_t, on_change_cb> m_cbs;

    RWMutexType m_mutex;
};  // 特定的模板类数据

/**
 * @brief ConfigVar的管理类
 * @details 提供便捷的方法创建/访问ConfigVar
 */
class Config{
public:
    using ConfigVarMap = std::unordered_map<std::string, ConfigVarBase::ptr>;
    using RWMutexType = RWMutex;

    /**
     * @brief 获取/创建对应参数名的配置参数
     * @param[in] name 配置参数名称
     * @param[in] default_value 参数默认值
     * @param[in] description 参数描述
     * @details 获取参数名为name的配置参数,如果存在直接返回
     *          如果不存在,创建参数配置并用default_value赋值
     * @return 返回对应的配置参数,如果参数名存在但是类型不匹配则返回nullptr
     * @exception 如果参数名包含非法字符[^0-9a-z_.] 抛出异常 std::invalid_argument
     */
    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name ,  const T& default_value, const std::string& description ){
        /* 如果参数名包含非法字符[^0-9a-z_.] 抛出异常 std::invalid_argument */
        if(name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != std::string::npos){
            WYZ_LOG_ERROR(WYZ_LOG_ROOT()) << "Lookup name invalid " << name;
            throw std::invalid_argument(name);
        }

        RWMutexType::WriteLock lock(GetRWMutex());
        auto it = GetDatas().find(name);
        if(it != GetDatas().end()){
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
            if(tmp){
                WYZ_LOG_INFO(WYZ_LOG_ROOT()) << "Lookup name="<<   name << "is exists";
                return tmp;
            }else {
                WYZ_LOG_ERROR(WYZ_LOG_ROOT()) << "Lookup name=" << name << " exists but type not "
                        << typeid(T).name() << " real_type=" << it->second->getTypeName()
                        << " " << it->second->toString();
                return nullptr;
            }
        }else {
            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name , description , default_value));
            GetDatas()[name] = v;
            return v;
        }
    }

    /**
     * @brief 使用YAML::Node初始化配置模块
     */
    static void LoadFromYaml(const YAML::Node& root);

    /**
     * @brief 查找配置参数,返回配置参数的基类
     * @param[in] name 配置参数名称
     */
    static ConfigVarBase::ptr LookupBase(const std::string& name);

    /**
     * @brief 遍历配置模块里面所有配置项
     * @param[in] cb 配置项回调函数
     */
    static void Visit(std::function<void(ConfigVarBase::ptr)>cb);

private:
    /**
     * @brief 返回所有的配置项
     */
    static ConfigVarMap& GetDatas() {
        static ConfigVarMap s_datas;
        return s_datas;
    }

    static RWMutexType& GetRWMutex(){
        static RWMutexType m_mutex;
        return m_mutex;
    };
};  // ConfigVar的管理类


}

#endif