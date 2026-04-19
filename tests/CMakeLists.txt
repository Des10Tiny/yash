include(FetchContent)

fetchcontent_declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
fetchcontent_makeavailable(googletest)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_executable(yashTests
    yash_main_tests.cpp
    tokenizer_tests.cpp
)

target_compile_definitions(yashTests PRIVATE UNIT_TEST)
target_link_libraries(yashTests
    PRIVATE
    yash_lib
    GTest::gtest_main
    GTest::gtest
)

if(MSVC)
    target_compile_options(yash_lib PRIVATE /W4 /WX)
else()
    target_compile_options(yash_lib PRIVATE -Wall -Wextra -Werror)
endif()

include(GoogleTest)
gtest_discover_tests(yashTests)
