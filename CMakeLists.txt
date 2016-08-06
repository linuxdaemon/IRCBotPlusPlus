cmake_minimum_required(VERSION 3.5)
project(IRCBot__)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp PracticalSocket.cpp PracticalSocket.h IRC.cpp IRC.h Util.cpp Util.h Config.cpp Config.h Module.cpp Module.h)
add_executable(IRCBot__ ${SOURCE_FILES})