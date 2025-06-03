cmake_minimum_required(VERSION 3.26)
project(numeric-methods LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/CPM.cmake)
include(cmake/Libraries.cmake)

add_subdirectory(lab6)
