cmake_minimum_required(VERSION 3.20)
project(WAMCer)

set(CMAKE_CXX_STANDARD 20)
add_executable(WAMCer
        main.cpp)
add_library(WAMCer-lib
        main.cpp)


# link smt-switch and gmp
set(SMT_SWITCH_DIR ${PROJECT_SOURCE_DIR}/smt-switch)

set(CMAKE_MODULE_PATH ${SMT_SWITCH_DIR}/cmake)
find_package(GMP REQUIRED)

target_link_libraries(WAMCer-lib PUBLIC
        ${SMT_SWITCH_DIR}/local/lib/libsmt-switch.a
        ${SMT_SWITCH_DIR}/local/lib/libsmt-switch-btor.a
        ${SMT_SWITCH_DIR}/local/lib/libsmt-switch-bitwuzla.a
        ${GMPXX_LIBRARIES}
        ${GMP_LIBRARIES})

target_include_directories(WAMCer-lib PUBLIC
        smt-switch/local/include
        ${GMP_INCLUDE_DIR})

target_link_libraries(WAMCer PUBLIC WAMCer-lib)

# add subdirectories
add_subdirectory(tests)