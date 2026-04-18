add_library(yash_lib STATIC
    tokenizer/tokenizer.cpp
)

if(MSVC)
    target_compile_options(yash_lib PRIVATE /W4 /WX)
else()
    target_compile_options(yash_lib PRIVATE -Wall -Wextra -Werror)
endif()

target_include_directories(yash_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(yash main.cpp)
target_link_libraries(yash PRIVATE yash_lib)
