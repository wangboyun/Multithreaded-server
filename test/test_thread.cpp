/*
 * @Description: thread模块测试 demo
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-09 15:09:41
 */

#include <ostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <iostream>
#include "../src/log.h"
#include "../src/thread.h"
#include "../src/config.h"

static wyz::Logger::ptr r_logger = WYZ_LOG_ROOT();

static int cout = 0;

static wyz::Mutex mutex;
wyz::RWMutex s_mutex;

void func1(){

    WYZ_LOG_INFO(r_logger) << "name=" << wyz::Thread::GetName() << " this name="<< wyz::Thread::GetThis()->getName() << " tid="<< wyz::GetThreadId() << " this tid="<< wyz::Thread::GetThis()->getId() << " "<< cout;
    for(int i = 0 ; i < 100000 ; ++i){ 
        // wyz::RWMutex::WriteLock lock(s_mutex);
        wyz::Mutex::Lock lock(mutex);
        cout++;
        std::cout<< cout << std::endl;
    }
}

void func2(){
    while (1) {
        WYZ_LOG_INFO(r_logger) << "xxxxxxxxxxxxxxxx";
    }
}

void func3(){
    while (1) {
        WYZ_LOG_INFO(r_logger) << "================";
    }
}
    
void test_log(){
    YAML::Node root = YAML::LoadFile("/home/wyz/workspace/wyz/bin/conf/log2.yml");
    wyz::Config::LoadFromYaml(root);

    std::vector<wyz::Thread::ptr> vec;
    // WYZ_LOG_INFO(r_logger) << "thread begin";
    for(int i = 0 ; i < 1; ++i){
        // wyz::Thread::ptr thr(new wyz::Thread(&func1,"name_"+ std::to_string(i)));
        // vec.emplace_back(thr);
        wyz::Thread::ptr thr2(new wyz::Thread(&func2 , "name_" + std::to_string(i + 1)));
        wyz::Thread::ptr thr3(new wyz::Thread(&func3 , "name_" + std::to_string(i + 2)));
        vec.emplace_back(thr2);
        vec.emplace_back(thr3);
    }
    
    for(size_t i = 0 ; i < vec.size() ; ++i){
        vec[i]->join();
    }
}

void test_cout(){
    std::vector<wyz::Thread::ptr> vec;
    WYZ_LOG_INFO(r_logger) << "thread begin";
    for(int i = 0 ; i < 4; ++i){
        wyz::Thread::ptr thr(new wyz::Thread(&func1,"name_"+ std::to_string(i)));
        vec.emplace_back(thr);
    }
    
    for(size_t i = 0 ; i < vec.size() ; ++i){
        vec[i]->join();
    }
    WYZ_LOG_INFO(r_logger) << "thread end ";

    std::cout <<"count = " << cout << std::endl;
}

int main(int argc , char* argv[]){

    
    // test_log();
    test_cout();
    
    return 0;
}