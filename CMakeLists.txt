cmake_minimum_required(VERSION 3.20)
project(WAMCer)

set(CMAKE_CXX_STANDARD 17)
add_executable(WAMCer
        main.cpp)
add_library(WAMCer-lib
        main.cpp
        frontends/btor2_encoder.cpp
        frontends/btorSim.cpp
        core/ts.cpp
        core/unroller.cpp
        core/runner.cpp
        core/solverFactory.cpp
        utils/logger.cpp
        utils/timer.cpp
        engines/bmc.cpp
        engines/k_induction.cpp
        engines/pdr.cpp
        engines/fbmc.cpp
        engines/filterWithSimulation.cpp
        deps/btor2tools/src/btorsim/btorsimbv.c
        deps/btor2tools/src/btorsim/btorsimrng.c
        deps/btor2tools/src/btorsim/btorsimstate.cpp
        deps/btor2tools/src/btorsim/btorsimam.cpp
        deps/btor2tools/src/btorsim/btorsimhelpers.cpp
        async/asyncTermSet.cpp
        engines/DirectConstructor.cpp
        async/asyncPreds.cpp
        engines/BMCChecker.cpp
        engines/InductionProver.cpp
        engines/InvConstructor.cpp
        engines/transitionFolder.cpp
        engines/PredCP.cpp
        engines/PredCP.h engines/AutoRestartBMC.cpp engines/AutoRestartBMC.h engines/InterpolantBmc.cpp engines/InterpolantBmc.h)


# link smt-switch and gmp, or boost
set(SMT_SWITCH_DIR ${PROJECT_SOURCE_DIR}/smt-switch)

set(CMAKE_MODULE_PATH ${SMT_SWITCH_DIR}/cmake)
find_package(GMP REQUIRED)
#find_package(Boost COMPONENTS thread chrono REQUIRED)

target_link_libraries(WAMCer-lib PUBLIC
        ${SMT_SWITCH_DIR}/local/lib/libsmt-switch-btor.a
        ${SMT_SWITCH_DIR}/local/lib/libsmt-switch-bitwuzla.a
        ${SMT_SWITCH_DIR}/local/lib/libsmt-switch-cvc5.a
        ${SMT_SWITCH_DIR}/local/lib/libsmt-switch-z3.a
        ${SMT_SWITCH_DIR}/local/lib/libsmt-switch.a
        ${PROJECT_SOURCE_DIR}/deps/btor2tools/build/lib/libbtor2parser.a
        pthread
        #        ${Boost_LIBRARIES}
        ${GMPXX_LIBRARIES}
        ${GMP_LIBRARIES})

#include_directories(${Boost_INCLUDE_DIRS})
ADD_DEFINITIONS("-DHAS_BOOST")

target_include_directories(WAMCer-lib PUBLIC
        smt-switch/local/include
        deps/btor2tools/src
        ${GMP_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR})

target_link_libraries(WAMCer PUBLIC WAMCer-lib)

# add subdirectories
add_subdirectory(tests)