cmake_minimum_required(VERSION 3.20)

project(crypto_signal_engine VERSION 0.1.0 LANGUAGES CXX)

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(BUILD_TESTING "Build unit and smoke tests" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

if(ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

add_library(crypto_core
    src/binance_feed.cpp
    src/binance_parser.cpp
    src/binance_rest.cpp
    src/config.cpp
    src/logger.cpp
    src/mean_reversion.cpp
    src/order_book.cpp
)

target_include_directories(crypto_core PUBLIC include ${Boost_INCLUDE_DIRS})
target_compile_definitions(crypto_core PUBLIC BOOST_ERROR_CODE_HEADER_ONLY BOOST_SYSTEM_NO_DEPRECATED)
target_link_libraries(crypto_core PUBLIC CURL::libcurl OpenSSL::SSL OpenSSL::Crypto Threads::Threads)

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

    add_executable(crypto_order_book_test tests/order_book_test.cpp)
    target_link_libraries(crypto_order_book_test PRIVATE crypto_core)
    add_test(NAME crypto_order_book_test COMMAND crypto_order_book_test)

    add_executable(crypto_rest_snapshot_test tests/binance_rest_test.cpp)
    target_link_libraries(crypto_rest_snapshot_test PRIVATE crypto_core)
    add_test(NAME crypto_rest_snapshot_test COMMAND crypto_rest_snapshot_test)

    add_executable(crypto_mean_reversion_test tests/mean_reversion_test.cpp)
    target_link_libraries(crypto_mean_reversion_test PRIVATE crypto_core)
    add_test(NAME crypto_mean_reversion_test COMMAND crypto_mean_reversion_test)
endif()