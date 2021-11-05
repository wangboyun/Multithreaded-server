/*
 * @Description: 测试地址模块
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-11-02 10:03:57
 */

#include "../src/address.h"
#include "../src/log.h"
#include <vector>

wyz::Logger::ptr g_logger = WYZ_LOG_ROOT();

void test_lookup(){
    std::vector<wyz::Address::ptr> addrs;
    wyz::Address::Lookup(addrs, "www.baidu.com:http");
    for(size_t i = 0; i < addrs.size(); ++i) {
        WYZ_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }
}

void test_iface(){
    std::multimap<std::string, std::pair<wyz::Address::ptr, uint32_t> > results;

    bool v = wyz::Address::GetInterfaceAddresses(results);
    if(!v) {
        WYZ_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return;
    }

    for(auto& i: results) {
        WYZ_LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << " - "
            << i.second.second;
    }
}

int main(){
    test_lookup();
    //test_iface();
    return 0;
}