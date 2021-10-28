cmake_minimum_required(VERSION 3.0.0)
project(wyz)

include (cmake/utils.cmake)
set(CMAKE_VERBOSE_MAKEFILE ON)

#添加编译 这个编译选项 CMAKE_CXX_FLAGS 表示c++,CMAKE_C_FLAGS表示c
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

include_directories(.)
#添加yaml库头文件位置
include_directories(/usr/local/include)
#添加boost库头文件位置
include_directories(/usr/include/boost)

#寻找yaml库路径
link_directories(/usr/local/lib)

#设置共享库
set(LIB_SRC 
    src/config.cpp
    src/fdmanager.cpp
    src/fiber.cpp
    src/hook.cpp
    src/iomanager.cpp
    src/log.cpp
    src/scheduler.cpp
    src/thread.cpp
    src/timer.cpp
    src/util.cpp  
)
    
#lib -- 共享库
add_library(wyz SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(wyz)      #编译共享库的时候，定义 _FILE_ 为相对路径 

#编译需要链接的库
set(LIBS 
    wyz
    dl
    pthread
    yaml-cpp
)

#添加可执行文件 测试日志模块
add_executable(test_log test/test_log.cpp )
#提醒编译器先生成wyz的共享库
add_dependencies(test_log wyz)
#链接编译所需的库文件
target_link_libraries(test_log ${LIBS})
force_redefine_file_macro_for_sources(test_log)

#可执行文件 测试配置模块
add_executable(test_config test/test_config.cpp )
add_dependencies(test_config wyz)
target_link_libraries(test_config ${LIBS})
force_redefine_file_macro_for_sources(test_config)

#可执行文件 测试线程模块
add_executable(test_thread test/test_thread.cpp )
add_dependencies(test_thread wyz)
target_link_libraries(test_thread ${LIBS})
force_redefine_file_macro_for_sources(test_thread)

#可执行文件 
add_executable(test_util test/test_util.cpp )
add_dependencies(test_util wyz)
target_link_libraries(test_util ${LIBS})
force_redefine_file_macro_for_sources(test_util)

#可执行文件 测试协程模块
add_executable(test_fiber test/test_fiber.cpp )
add_dependencies(test_fiber wyz)
target_link_libraries(test_fiber ${LIBS})
force_redefine_file_macro_for_sources(test_fiber)

#可执行文件 测试协程调度模块
add_executable(test_scheduler test/test_scheduler.cpp )
add_dependencies(test_scheduler wyz)
target_link_libraries(test_scheduler ${LIBS})
force_redefine_file_macro_for_sources(test_scheduler)

#可执行文件 测试IO协程调度模块
add_executable(test_iomanager test/test_iomanager.cpp )
add_dependencies(test_iomanager wyz)
target_link_libraries(test_iomanager ${LIBS})
force_redefine_file_macro_for_sources(test_iomanager)

#可执行文件 测试IO hook模块
add_executable(test_hook test/test_hook.cpp )
add_dependencies(test_hook wyz)
target_link_libraries(test_hook ${LIBS})
force_redefine_file_macro_for_sources(test_hook)


#将可执行文件放在本文件的根目录下bin文件夹下
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#将库文件放到根目录下的lib文件夹下
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
