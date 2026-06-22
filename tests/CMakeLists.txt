include(FetchContent)

fetchcontent_declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest
    GIT_TAG v2.5.2
)
fetchcontent_makeavailable(doctest)


set(TEST_SOURCES
    doctest_main.cpp
    tokenizer_tests.cpp
    expander_test.cpp
    parser_tests.cpp
    scoped_pipe_test.cpp
    unique_fd_test.cpp
    config_parser_test.cpp
    executor_base_test.cpp
    executor_redirect_tests.cpp
    yash_base_tests.cpp
)

include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")

if(BUILD_TESTS)
    add_executable(yashTests ${TEST_SOURCES})
    target_compile_definitions(yashTests PRIVATE UNIT_TEST)
    target_include_directories(yashTests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests)
    target_link_libraries(yashTests PRIVATE yash_lib doctest::doctest)

    doctest_discover_tests(yashTests TEST_SUFFIX "_Base")
endif()



if(BUILD_TESTS_SANITIZED)
    add_executable(yashTestsSanitize ${TEST_SOURCES})
    target_compile_definitions(yashTestsSanitize PRIVATE UNIT_TEST)

    target_include_directories(yashTestsSanitize PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests)
    target_compile_options(yashTestsSanitize PRIVATE -fsanitize=address,undefined -g -fno-omit-frame-pointer)
    target_link_options(yashTestsSanitize PRIVATE -fsanitize=address,undefined)
    target_link_libraries(yashTestsSanitize PRIVATE yash_lib_sanitized doctest::doctest)

    doctest_discover_tests(yashTestsSanitize TEST_SUFFIX "_BaseWithSanitizer")
endif()




