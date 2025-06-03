cmake_minimum_required(VERSION 3.26)
project(numeric-methods-lab-6 LANGUAGES CXX VERSION 0.1.0)

add_executable(
        num-method-lab6
        main.cpp
)

if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang|GNU")
    target_compile_options(num-method-lab6 PRIVATE -march=native)
endif ()

isl_set_target_flags(
        num-method-lab6
       OFF
        ""
        ""
        ""
        ""
        OFF
        OFF
        ON
)

target_link_libraries(
        num-method-lab6
        PUBLIC
        math-visualizer
)
