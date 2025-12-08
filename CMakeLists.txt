cmake_minimum_required(VERSION 3.27)
project(DSM)

set(CMAKE_CXX_STANDARD 17)

add_executable(DSM main.cpp
        include/dsm/ObjectStore.h
        include/net/DsmNetwork.h)
