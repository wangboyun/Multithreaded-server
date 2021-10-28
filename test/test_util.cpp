/*
 * @Description: 
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-10-12 10:55:35
 */

#include "../src/util.h"
#include "../src/macro.h"
#include "../src/log.h"

static wyz::Logger::ptr g_logger = WYZ_LOG_ROOT();
void test_assert(){
    // WYZ_ASSERT(0);
    WYZ_ASSERT2(0 == 1 , "ssss");
}

int main(int argc , char** argv){
    test_assert();
}