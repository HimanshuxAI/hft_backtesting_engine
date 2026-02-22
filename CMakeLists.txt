cmake_minimum_required(VERSION 3.14)
project(hft_backtest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Performance flags
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -march=native -DNDEBUG")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/csv_parser.cpp
    src/order_book.cpp
    src/matching_engine.cpp
    src/latency_model.cpp
    src/mean_reversion_strategy.cpp
    src/backtester.cpp
    src/risk_metrics.cpp
)

# Executable
add_executable(hft_backtest ${SOURCES})

# Link pthreads
find_package(Threads REQUIRED)
target_link_libraries(hft_backtest PRIVATE Threads::Threads)

# Compiler warnings
target_compile_options(hft_backtest PRIVATE -Wall -Wextra -Wpedantic)
