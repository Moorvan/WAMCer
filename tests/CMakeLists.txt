include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
FetchContent_MakeAvailable(googletest)


enable_testing()

add_executable( solver_test
        solver_test.cpp)

target_link_libraries( solver_test
        gtest_main
        WAMCer-lib)

include(GoogleTest)
gtest_discover_tests(
        solver_test)

include_directories(../)