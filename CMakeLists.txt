cmake_minimum_required(VERSION 3.16)

project(openflash_protocol LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# OpenFlash core

add_library(openflash_core STATIC
    openflash_esp32/src/protocol.cpp
    openflash_esp32/src/packet_stream_parser.cpp
    openflash_esp32/src/request_handler.cpp
    openflash_esp32/src/utilities.cpp
)

target_include_directories(openflash_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/openflash_esp32/src
)

# Request sender

add_library(openflash_sender STATIC
    sender/src/request_sender.cpp
    sender/src/mock_filesystem.cpp
)

target_include_directories(openflash_sender PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/sender/include
)

target_link_libraries(openflash_sender PUBLIC
    openflash_core
)

# Native tests

add_executable(test_openflash_protocol
    tests/test_protocol.cpp
)

target_link_libraries(test_openflash_protocol PRIVATE
    openflash_core
    openflash_sender
)

if(MSVC)
    target_compile_options(openflash_core PRIVATE
        /W4
        /permissive-
    )

    target_compile_options(openflash_sender PRIVATE
        /W4
        /permissive-
    )

    target_compile_options(test_openflash_protocol PRIVATE
        /W4
        /permissive-
    )
else()
    target_compile_options(openflash_core PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

    target_compile_options(openflash_sender PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

    target_compile_options(test_openflash_protocol PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# CTest

enable_testing()

add_test(
    NAME openflash_protocol
    COMMAND test_openflash_protocol
)