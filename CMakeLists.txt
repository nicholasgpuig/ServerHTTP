# CMakeList.txt : CMake project for ServerHTTP, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.20)
project(ServerHTTP)

add_executable(ServerHTTP ServerHTTP.cpp socket.cpp)

set_property(TARGET ServerHTTP PROPERTY CXX_STANDARD 20)
set_property(TARGET ServerHTTP PROPERTY CXX_STANDARD_REQUIRED ON)

if(WIN32)
    target_include_directories(ServerHTTP PRIVATE
        "\\\\wsl.localhost\\Ubuntu\\usr\\include"
        "\\\\wsl.localhost\\Ubuntu\\usr\\include\\x86_64-linux-gnu"
    )
endif()

# TODO: Add tests and install targets if needed.
