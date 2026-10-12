cmake_minimum_required(VERSION 3.20)

project(crypto_signal_engine VERSION 0.1.0 LANGUAGES CXX)

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(BUILD_TESTING "Build unit and smoke tests" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

add_library(crypto_core
    src/binance_parser.cpp
    src/config.cpp
    src/logger.cpp
)

target_include_directories(crypto_core PUBLIC include)

if(ENABLE_WARNINGS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(crypto_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

add_executable(crypto_signal_engine src/main.cpp)
target_link_libraries(crypto_signal_engine PRIVATE crypto_core)

if(ENABLE_WARNINGS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(crypto_signal_engine PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(crypto_smoke_test tests/smoke_test.cpp)
    target_link_libraries(crypto_smoke_test PRIVATE crypto_core)
    add_test(NAME crypto_smoke_test COMMAND crypto_smoke_test)

    add_executable(crypto_contracts_queue_test tests/contracts_queue_test.cpp)
    target_link_libraries(crypto_contracts_queue_test PRIVATE crypto_core)
    add_test(NAME crypto_contracts_queue_test COMMAND crypto_contracts_queue_test)

    add_executable(crypto_binance_parser_test tests/binance_parser_test.cpp)
    target_link_libraries(crypto_binance_parser_test PRIVATE crypto_core)
    add_test(NAME crypto_binance_parser_test COMMAND crypto_binance_parser_test)
endif()