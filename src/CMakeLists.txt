set(YASH_SOURCES
    tokenizer/tokenizer.cpp
    parser/parser.cpp
    executor/executor.cpp
    utils/config_parser.cpp
    utils/logger.cpp
)

add_library(yash_lib STATIC ${YASH_SOURCES})
target_include_directories(yash_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(yash_lib PRIVATE /W4 /WX)
else()
    target_compile_options(yash_lib PRIVATE -Wall -Wextra -Werror)
endif()


add_executable(yash main.cpp)
target_link_libraries(yash PRIVATE yash_lib)

if(BUILD_TESTS_SANITIZED)
    add_library(yash_lib_sanitized STATIC ${YASH_SOURCES})
    target_include_directories(yash_lib_sanitized PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    if(MSVC)
        target_compile_options(yash_lib_sanitized PRIVATE /W4 /WX)
    else()
        target_compile_options(yash_lib_sanitized PRIVATE -Wall -Wextra -Werror)
    endif()

    target_compile_options(yash_lib_sanitized PRIVATE -fsanitize=address,undefined -g -fno-omit-frame-pointer)
    target_link_options(yash_lib_sanitized PRIVATE -fsanitize=address,undefined)
endif()
