cmake_minimum_required(VERSION 3.28)
project(kuznechik)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O3 -Ofast -Wall -Wother -flto -march=native -ffast-math -funroll-loops")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

add_executable(kuznechik main.cpp
        kuznyechik.hpp
        kuznyechik.cpp
        block128.hpp
        block128.cpp)
