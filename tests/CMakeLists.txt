include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
FetchContent_MakeAvailable(googletest)


enable_testing()

add_executable( wamcerTests
        solver_test.cpp
        engines_test.cpp
        utils_test.cpp
        runner_test.cpp
        cases_test.cpp
        cases/counter.cpp
        hwmcc_test.cpp learning_test.cpp)

target_link_libraries( wamcerTests
        gtest_main
        WAMCer-lib)

include(GoogleTest)
gtest_discover_tests(
        wamcerTests)

include_directories(
        ../
        ./
        )