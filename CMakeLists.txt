cmake_minimum_required(VERSION 3.16)
project(cpp_concurrent_order_engine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build tests" OFF)

add_library(order_engine
  src/order_engine.cpp
)
target_include_directories(order_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (MSVC)
  target_compile_options(order_engine PRIVATE /W4 /permissive-)
else()
  target_compile_options(order_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(order_engine_demo src/main.cpp)
target_link_libraries(order_engine_demo PRIVATE order_engine)

if (BUILD_TESTS)
  enable_testing()
  add_executable(test_determinism tests/test_determinism.cpp)
  target_link_libraries(test_determinism PRIVATE order_engine)
  add_test(NAME determinism_tests COMMAND test_determinism)
endif()
