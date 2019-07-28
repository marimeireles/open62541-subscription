cmake_minimum_required(VERSION 3.5)
project(OpcUaConsoleClient)

set(CMAKE_VERBOSE_MAKEFILE ON)

add_subdirectory(open62541)

set(SERVER server.c)
set(CLIENT client.c)

set(CMAKE_CXX_STANDARD 11)

add_executable(OpcUaConsoleServer ${SERVER})
target_link_libraries(OpcUaConsoleServer open62541 readline ${Boost_LIBRARIES})
add_executable(OpcUaConsoleClient ${CLIENT})
target_link_libraries(OpcUaConsoleClient open62541 readline ${Boost_LIBRARIES})