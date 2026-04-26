cmake_minimum_required(VERSION 3.25)
project(yash)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(CMAKE_EXPORT_COMPILE_COMMANDS AND UNIX)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    )
endif()

option(USE_SANITIZERS "Enable Address and UndefinedBehavior sanitizers" OFF)

if(USE_SANITIZERS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined -g -O1 -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")
endif()

option(BUILD_TESTS "Build all unit tests" OFF)
option(BUILD_TESTS_SANITIZED "Build all unit tests with sanitizers" OFF)

add_subdirectory(src)

if(CMAKE_BUILD_TYPE MATCHES "Release")
    cmake_policy(SET CMP0069 NEW)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT error)

    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()

endif()

if(BUILD_TESTS OR BUILD_TESTS_SANITIZED)
    enable_testing()
    add_subdirectory(tests)
endif()

find_program(CLANG_FORMAT_EXE clang-format)
if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE FORMAT_SOURCES "src/*.cpp" "src/*.hpp" "tests/*.cpp")
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i ${FORMAT_SOURCES}
        COMMENT "Running clang-format on all C++ source files..."
    )
endif()
