cmake_minimum_required(VERSION 2.8)
set(CMAKE_C_COMPILER  "/opt/EmbedSky/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc")
set(CMAKE_CXX_COMPILER "/opt/EmbedSky/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++")
set(CMAKE_CXX_FLAGS "-std=c++11 -pthread")

INCLUDE_DIRECTORIES("/home/zhangshaoyan/armbuild/opencv3.4.1.for.arm/include")
LINK_DIRECTORIES("/home/zhangshaoyan/armbuild/opencv3.4.1.for.arm/lib")

project(IMP4ARM)
add_executable(${PROJECT_NAME} "main.cpp" "zcamdevice.cpp" "zgblpara.h")
