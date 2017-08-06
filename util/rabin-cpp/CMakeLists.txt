cmake_minimum_required(VERSION 3.2)
cmake_policy(SET CMP0028 NEW)
cmake_policy(SET CMP0048 NEW)

add_library(rabin-cpp Rabin.cpp Rabin.hpp)
target_include_directories(rabin-cpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_property(TARGET rabin-cpp PROPERTY CXX_STANDARD 11)
set_property(TARGET rabin-cpp PROPERTY CXX_STANDARD_REQUIRED TRUE)

find_package(Boost 1.58 REQUIRED)
target_link_libraries(rabin-cpp PUBLIC Boost::boost)
