include(FetchContent)

fetchcontent_declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
fetchcontent_makeavailable(googletest)

set(TEST_SOURCES
    yash_main_tests.cpp
    tokenizer_tests.cpp
    parser_tests.cpp
    scoped_fd_test.cpp
    config_parser_test.cpp
    executor_base_test.cpp
)

include(GoogleTest)

if(BUILD_TESTS)
    add_executable(yashTests ${TEST_SOURCES})
    target_compile_definitions(yashTests PRIVATE UNIT_TEST)
    target_include_directories(yashTests PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(yashTests PRIVATE yash_lib GTest::gtest_main GTest::gtest)

    gtest_discover_tests(yashTests TEST_SUFFIX "_Base")
endif()



if(BUILD_TESTS_SANITIZED)
    add_executable(yashTestsSanitize ${TEST_SOURCES})
    target_compile_definitions(yashTestsSanitize PRIVATE UNIT_TEST)

    target_include_directories(yashTestsSanitize PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_compile_options(yashTestsSanitize PRIVATE -fsanitize=address,undefined -g -fno-omit-frame-pointer)
    target_link_options(yashTestsSanitize PRIVATE -fsanitize=address,undefined)
    target_link_libraries(yashTestsSanitize PRIVATE yash_lib_sanitized GTest::gtest_main GTest::gtest)

    gtest_discover_tests(yashTestsSanitize TEST_SUFFIX "_BaseWithSanitizer")
endif()




