/*
 * @Description: 
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-09-24 14:21:54
 */

#include "../src/config.h"
#include "../src/log.h"
#include <cstddef>
#include <exception>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <yaml-cpp/yaml.h>
#include <yaml-cpp/node/node.h>


wyz::ConfigVar<int>::ptr g_int_val_config = wyz::Config::Lookup("system.port",(int)8080,"端口号");
wyz::ConfigVar<float>::ptr g_float_val_config = wyz::Config::Lookup("system.port",(float)8080,"端口号");

wyz::ConfigVar<std::vector<int>>::ptr g_int_vector_config = wyz::Config::Lookup("system.vector",\
    std::vector<int>{1,3}, " vector int type");
wyz::ConfigVar<std::list<int>>::ptr g_int_list_config = wyz::Config::Lookup("system.list",\
    std::list<int>{1,3}, " vector int list");
wyz::ConfigVar<std::set<int>>::ptr g_int_set_config = wyz::Config::Lookup("system.set",\
    std::set<int>{1,3}, " vector int set");

wyz::ConfigVar<std::unordered_set<int>>::ptr g_int_uset_config = wyz::Config::Lookup("system.uset",\
    std::unordered_set<int>{1,3}, " vector int uset");

wyz::ConfigVar<std::map<std::string,int>>::ptr g_int_map_config = wyz::Config::Lookup("system.map",\
    std::map<std::string,int>{{"key",3}}, " vector int map");

wyz::ConfigVar<std::unordered_map<std::string,int>>::ptr g_int_umap_config = wyz::Config::Lookup("system.umap",\
    std::unordered_map<std::string,int>{{"key",3}}, " vector int umap");

/* 遍历输出 xx.yaml 文件数据 */
void print_yaml(YAML::Node node , int level){
    if(node.IsScalar()){                // 是纯量
        WYZ_LOG_INFO(WYZ_LOG_ROOT()) << std::string(level*4 , ' ') <<
        node.Scalar() << "-" << node.Type() << "-" << level;
    }
    else if (node.IsSequence()) {       // 是数组
        for(size_t i = 0 ; i < node.size() ; i++){
            WYZ_LOG_INFO(WYZ_LOG_ROOT()) << std::string(level*4 , ' ') <<
                i << "-" << node.Type() << "-" << level;
            print_yaml(node[i], level+1);
        }
    }
    else if (node.IsMap()) {           // 是map 
        for(auto it = node.begin() ; it != node.end() ; ++it){
            WYZ_LOG_INFO(WYZ_LOG_ROOT()) << std::string(level*4 , ' ') <<
                it->first << "-" << node.Type() << "-" << level;
            print_yaml(it->second, level+1);
        }
    }
    else if (node.IsNull()) {          // 是空
        WYZ_LOG_INFO(WYZ_LOG_ROOT()) <<  "NULL -" << node.Type() << "-" << level;
    }
}


void test_yaml(void){
    try {
        YAML::Node node = YAML::LoadFile("/home/wyz/workspace/wyz/bin/conf/test.yaml");
        print_yaml(node,0);
    } catch (YAML::Exception& err) {
        WYZ_LOG_ERROR(WYZ_LOG_ROOT())<< "yaml loadfile err" << err.what();
    }
    
    //WYZ_LOG_INFO(WYZ_LOG_DEFUALTLOG()) << node;
    
}


namespace wyz {

/* 自定义数据类型 需要模板类片例化 */
class Person{
public:
    
    std::string toString()const{
        std::stringstream ss;
        ss << "[Person name=" << name
           << " age=" << age
           << "]";
        return ss.str();
    }
    bool operator== (const Person& oth) const {
        return name == oth.name
            && age == oth.age;
    }
    bool operator< (const Person& oth)const {
        return name < oth.name;
    }

    std::string getname()const      {return name;}
    int getage()const               {return age;}
    void setname(const std::string& cname)      { name = cname;}
    void setage(const int cage)                 { age = cage;}

private:
    std::string name = "";
    int age = 0;
};

// 模板类实例化  
template<>           
class LexicalCast<Person, std::string>{
public:
    std::string operator() (const Person & p){
        YAML::Node node;
        std::stringstream ss;
        node["name"] = p.getname();
        node["age"] = p.getage();
        ss << node;
        return ss.str();
    }
};

// 模板类实例化   
template<>
class LexicalCast<std::string, Person>{
public:
    Person operator() (const std::string& str){
        YAML::Node node = YAML::Load(str);        
        Person p;
        std::stringstream ss;
        p.setname(node["name"].as<std::string>());
        p.setage(node["age"].as<int>());
        return  p;
    }
};

}

wyz::ConfigVar<wyz::Person>::ptr g_person = wyz::Config::Lookup("class.person",wyz::Person(),"人类");
wyz::ConfigVar<std::map<std::string, wyz::Person> >::ptr g_person_map =
    wyz::Config::Lookup("class.map", std::map<std::string, wyz::Person>(), "system person");

void test_type(){
    g_person->addListener([](const wyz::Person& old_value, const wyz::Person& new_value){
        WYZ_LOG_INFO(WYZ_LOG_ROOT()) << "old_value=" << old_value.toString()
                << " new_value=" << new_value.toString();
    });

    WYZ_LOG_INFO(WYZ_LOG_ROOT()) << "before: "  << g_person->toString();

    YAML::Node node = YAML::LoadFile("/home/wyz/workspace/wyz/bin/conf/test.yml");
    wyz::Config::LoadFromYaml(node);
    
    WYZ_LOG_INFO(WYZ_LOG_ROOT()) << "after: "  << g_person->toString();
}

void test_log(){
    static wyz::Logger::ptr system_log = WYZ_LOG_NAME("system");
    WYZ_LOG_INFO(system_log) << " hello system";

    std::cout << wyz::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node root = YAML::LoadFile("/home/wyz/workspace/wyz/bin/conf/log.yml");
    wyz::Config::LoadFromYaml(root);

    std::cout << "=============" << std::endl;
    std::cout << wyz::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    std::cout << "=============" << std::endl;
    std::cout << root << std::endl;
    WYZ_LOG_INFO(system_log) << "hello system" << std::endl;

    system_log->setFormatter("%d - %N%T%m%n");
    WYZ_LOG_INFO(system_log) << "hello system" << std::endl;
    
}

int main(int agrc, char** argv){
    
    // test_yaml();
    //test_config();
    //test_type();
    test_log();
    wyz::Config::Visit([](wyz::ConfigVarBase::ptr var){
        WYZ_LOG_INFO(WYZ_LOG_ROOT()) << "name=" << var->getName()
                    << " description=" << var->getDescrip()
                    << " typename=" << var->getTypeName()
                    << " value=" << var->toString();
    });
    return 0;
    
}