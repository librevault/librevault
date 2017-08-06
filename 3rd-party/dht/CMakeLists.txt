cmake_minimum_required(VERSION 3.0)
project(dht)

#============================================================================
# Internal compiler options
#============================================================================

option(DHT_BUILD_EXAMPLE "Build DHT example code" OFF)

#============================================================================
# Compile targets
#============================================================================

add_library(${PROJECT_NAME} STATIC dht.c dht.h)
target_include_directories(dht PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(DHT_BUILD_EXAMPLE)
	add_executable(${PROJECT_NAME}-example dht-example.c)
	target_link_libraries(${PROJECT_NAME}-example dht)
	target_link_libraries(${PROJECT_NAME}-example crypt)
endif()