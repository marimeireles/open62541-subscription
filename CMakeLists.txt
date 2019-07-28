cmake_minimum_required(VERSION 3.5)
project(OpcUaConsoleClient)

set(CMAKE_VERBOSE_MAKEFILE ON)

## globally use C++11 within the project
set(CMAKE_CXX_STANDARD 11)

## load in pkg-config support
find_package(PkgConfig)
## look for the libzmq
pkg_check_modules(LIBZMQ REQUIRED libzmq)

add_subdirectory(open62541)

set(SERVER server.c)
set(ZMQ_EMITTER zmq_emitter.c)
set(CLIENT client.c)

add_executable(OpcUaConsoleServer ${SERVER})
target_link_libraries(OpcUaConsoleServer open62541 readline ${Boost_LIBRARIES} ${LIBZMQ_LINK_LIBRARIES})
target_include_directories(OpcUaConsoleServer PUBLIC ${LIBZMQ_INCLUDE_DIRS})
target_compile_options(OpcUaConsoleServer PUBLIC ${LIBZMQ_CFLAGS})

add_executable(OpcUaConsoleEmitter ${ZMQ_EMITTER})
target_link_libraries(OpcUaConsoleEmitter open62541 readline ${Boost_LIBRARIES} ${LIBZMQ_LINK_LIBRARIES})
target_include_directories(OpcUaConsoleEmitter PUBLIC ${LIBZMQ_INCLUDE_DIRS})
target_compile_options(OpcUaConsoleEmitter PUBLIC ${LIBZMQ_CFLAGS})

add_executable(OpcUaConsoleClient ${CLIENT})
target_link_libraries(OpcUaConsoleClient open62541 readline ${Boost_LIBRARIES})
