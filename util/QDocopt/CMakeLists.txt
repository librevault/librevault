cmake_minimum_required(VERSION 3.2)
cmake_policy(SET CMP0028 NEW)
cmake_policy(SET CMP0048 NEW)

add_library(QDocopt INTERFACE)
target_include_directories(QDocopt INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(QDocopt INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/QDocopt.hpp)

find_package(Qt5 5.0 REQUIRED COMPONENTS Core)
target_link_libraries(QDocopt INTERFACE Qt5::Core)

target_link_libraries(QDocopt INTERFACE CONAN_PKG::docopt.cpp)
