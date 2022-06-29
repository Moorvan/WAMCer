cmake_minimum_required(VERSION 3.20)
project(WAMCer)

set(CMAKE_CXX_STANDARD 20)
add_executable(WAMCer main.cpp)

target_link_libraries(WAMCer PUBLIC ${PROJECT_SOURCE_DIR}/smt-switch/local/lib/libsmt-switch.a)
target_link_libraries(WAMCer PUBLIC ${PROJECT_SOURCE_DIR}/smt-switch/local/lib/libsmt-switch-btor.a)
target_include_directories(WAMCer PUBLIC
        smt-switch/local/include)
