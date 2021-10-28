/*
 * @Description: 
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-04 11:17:02
 */
 
#include <string>
#include "config.h"
#include "log.h"
#include <list>


namespace wyz {

/* YAML 类型*/
//A:
//  B: 10
//  C: str
// --- "A.B" 10

static void ListAllMember(const std::string& prefix,const YAML::Node& node , std::list<std::pair<std::string, YAML::Node>>& output) {
    if(prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos){
        WYZ_LOG_ERROR(WYZ_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
        return;
    }
    output.emplace_back(std::make_pair(prefix, node));
    if(node.IsMap()){
        for(auto it = node.begin(); it != node.end() ; ++it){
            if(prefix.empty())
                ListAllMember(it->first.Scalar(), it->second,output);
            else
                ListAllMember(prefix + "." + it->first.Scalar(), it->second, output);
        }
    }
}

/**
* @brief 使用YAML::Node初始化配置模块
*/
void Config::LoadFromYaml(const YAML::Node& root){
    std::list<std::pair<std::string , YAML::Node>> all_nodes;
    ListAllMember("", root, all_nodes);
    for(auto i : all_nodes){
        std::string key = i.first;
        std::transform(key.begin(),key.end(),key.begin(),::tolower);
        ConfigVarBase::ptr var = LookupBase(key);
        if(var){
            if(i.second.IsScalar()){
                var->fromString(i.second.Scalar());
            }else {
                std::stringstream ss;
                ss << i.second;
                var->fromString(ss.str());
            }
        }
    }
}

/**
* @brief 查找配置参数,返回配置参数的基类
* @param[in] name 配置参数名称
*/
ConfigVarBase::ptr Config::LookupBase(const std::string& name){
    RWMutexType::ReadLock lock(GetRWMutex());
    auto it = GetDatas().find(name);
    if(it != GetDatas().end())
        return it->second;
    return nullptr;
}

/**
* @brief 遍历配置模块里面所有配置项
* @param[in] cb 配置项回调函数
*/
void Config::Visit(std::function<void (ConfigVarBase::ptr)> cb){
    RWMutexType::ReadLock lock(GetRWMutex());
    for(auto it = GetDatas().begin() ; it != GetDatas().end() ; ++it){
        cb(it->second);
    }
}


}