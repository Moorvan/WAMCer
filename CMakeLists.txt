cmake_minimum_required(VERSION 3.20)
project(WAMCer)

set(CMAKE_CXX_STANDARD 20)
add_executable(WAMCer
        main.cpp)
add_library(WAMCer-lib
        main.cpp
        frontends/btor2_encoder.cpp
        core/ts.cpp
        core/unroller.cpp
        core/runner.cpp
        utils/logger.cpp
        utils/timer.cpp
        engines/bmc.cpp
        engines/k_induction.cpp
        engines/pdr.cpp
        )


# link smt-switch and gmp
set(SMT_SWITCH_DIR ${PROJECT_SOURCE_DIR}/smt-switch)

set(CMAKE_MODULE_PATH ${SMT_SWITCH_DIR}/cmake)
find_package(GMP REQUIRED)

target_link_libraries(WAMCer-lib PUBLIC
        ${SMT_SWITCH_DIR}/local/lib/libsmt-switch.a
        ${SMT_SWITCH_DIR}/local/lib/libsmt-switch-btor.a
        ${SMT_SWITCH_DIR}/local/lib/libsmt-switch-bitwuzla.a
#        ${SMT_SWITCH_DIR}/local/lib/libsmt-switch-z3.a
        ${GMPXX_LIBRARIES}
        ${GMP_LIBRARIES})

target_include_directories(WAMCer-lib PUBLIC
        smt-switch/local/include
        smt-switch/deps/boolector/deps/btor2tools/src
        ${GMP_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR})

target_link_libraries(WAMCer PUBLIC WAMCer-lib)

# add subdirectories
add_subdirectory(tests)