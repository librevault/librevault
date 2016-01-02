project(librevault)
cmake_minimum_required(VERSION 3.2)

#============================================================================
# Settable options
#============================================================================

option(WITH_COTIRE "Use cotire for builds." ON)
option(BUILD_DOCUMENTATION "Use Doxygen to create the HTML based API documentation" OFF)

#============================================================================
# CMake modules
#============================================================================

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
if(WITH_COTIRE)
	include(cotire)
endif()
include(GNUInstallDirs)

#============================================================================
# Internal compiler options
#============================================================================

# Setting compiler properties
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(MSVC)
	# Force to always compile with W4
	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	# Update if necessary
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long")
endif()

if(CMAKE_COMPILER_IS_GNUCXX AND NOT WIN32)
	# Use -flto flag to enable GCC's link-time optimization.
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
	set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} -flto")
	if(MINGW)
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -fno-use-linker-plugin")
		set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} -fno-use-linker-plugin")
	endif()
endif()

option(BUILD_STATIC "Build static version of executable" OFF)
if(BUILD_STATIC)
	set(CMAKE_CXX_FLAGS "-static -static-libgcc -static-libstdc++ ${CMAKE_CXX_FLAGS}")
	set(CMAKE_C_FLAGS "-static -static-libgcc -static-libstdc++ ${CMAKE_C_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++ ${CMAKE_EXE_LINKER_FLAGS}")
	set(CMAKE_MODULE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++ ${CMAKE_MODULE_LINKER_FLAGS}")
	set(Boost_USE_STATIC_LIBS ON)
	set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()

if(WIN32)
	add_definitions(-D_WIN32_WINNT=0x600)
	add_definitions(-DUNICODE)
endif()

#============================================================================
# Sources & headers
#============================================================================

list(APPEND SRCS "src/pch.h")
# Main sources
file(GLOB_RECURSE MAIN_SRCS "src/*.cpp")
file(GLOB_RECURSE MAIN_HEADERS "src/*.h")
list(APPEND SRCS ${MAIN_SRCS})
list(APPEND SRCS ${MAIN_HEADERS})

include(GetGitRevisionDescription)
git_describe(LV_APPVER)
configure_file("appver.in.h" "appver.h")

#============================================================================
# Compile targets
#============================================================================

add_executable(${PROJECT_NAME} ${SRCS})

if(WITH_COTIRE)
	cotire(${PROJECT_NAME})
endif()

#============================================================================
# Third-party libraries
#============================================================================

# Cryptodiff
find_package(Cryptodiff REQUIRED)
target_link_libraries(${PROJECT_NAME} cryptodiff-shared)

# Pthreads
find_package(Threads)
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

# Docopt
add_subdirectory("contrib/docopt")
#include_directories("contrib/docopt")
target_link_libraries(${PROJECT_NAME} docopt_s)

# Boost
set(Boost_COMPONENTS
		system
		filesystem
		iostreams
		thread)
set(Boost_USE_MULTITHREADED ON)

find_package(Boost REQUIRED COMPONENTS ${Boost_COMPONENTS})
target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
# /Boost

# cpp-netlib
#find_package(cppnetlib 0.11.1 REQUIRED)
#include_directories(${CPPNETLIB_INCLUDE_DIRS})
#link_libraries(${CPPNETLIB_LIBRARIES})

# cURL
find_package(CURL)
target_include_directories(${PROJECT_NAME} PRIVATE ${CURL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${CURL_LIBRARIES})

# SQLite
find_package(Sqlite3)

file(GLOB_RECURSE LVSQLITE3_SRCS "contrib/include/lvsqlite3/*")
add_library(lvsqlite3-bundled STATIC ${LVSQLITE3_SRCS})
target_include_directories(lvsqlite3-bundled PUBLIC "contrib/include")

if(SQLITE3_FOUND)
	target_include_directories(lvsqlite3-bundled PUBLIC ${SQLITE3_INCLUDE_DIR})
	target_link_libraries(lvsqlite3-bundled ${SQLITE3_LIBRARIES})
else()
	add_library(sqlite3-bundled STATIC contrib/sqlite3/sqlite3.c)
	target_include_directories(lvsqlite3-bundled PUBLIC "contrib/sqlite3")
	target_link_libraries(lvsqlite3-bundled sqlite3-bundled)
endif()

target_link_libraries(${PROJECT_NAME} lvsqlite3-bundled)
# /SQLite

# Protobuf
find_package(Protobuf REQUIRED)

file(GLOB_RECURSE PROTO_LIST "src/*.proto")
protobuf_generate_cpp(PROTO_SOURCES PROTO_HEADERS ${PROTO_LIST})

add_library(librevault-protobuf STATIC ${PROTO_SOURCES} ${PROTO_HEADERS})
target_include_directories(librevault-protobuf PUBLIC ${PROTOBUF_INCLUDE_DIRS})
target_link_libraries(librevault-protobuf PUBLIC ${PROTOBUF_LIBRARIES})

target_link_libraries(${PROJECT_NAME} librevault-protobuf)

if(WITH_COTIRE)
	set_source_files_properties("src/Version.cpp" PROPERTIES COTIRE_EXCLUDED ON)	# As it changes every commit
	set_source_files_properties(${PROTO_SOURCES} PROPERTIES COTIRE_EXCLUDED ON)
endif()

# CryptoPP
find_package(CryptoPP 5.6.2 REQUIRED)
target_link_libraries(${PROJECT_NAME} cryptopp-shared)

# libnatpmp
find_library(libnatpmp_LIBRARIES natpmp)
target_link_libraries(${PROJECT_NAME} ${libnatpmp_LIBRARIES})

if(WIN32)
	add_definitions(-DNATPMP_STATICLIB)
endif()

# OpenSSL
find_package(OpenSSL REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})

# WinSock
if(WIN32)
	target_link_libraries(${PROJECT_NAME} wsock32)
	target_link_libraries(${PROJECT_NAME} ws2_32)
	target_link_libraries(${PROJECT_NAME} Iphlpapi)
endif()

if(BUILD_STATIC)
	if(UNIX)
		target_link_libraries(${PROJECT_NAME} dl)
		target_link_libraries(${PROJECT_NAME} c.a)
	endif()
	target_link_libraries(${PROJECT_NAME} stdc++.a)
	target_link_libraries(${PROJECT_NAME} m.a)
	target_link_libraries(${PROJECT_NAME} pthread.a)
endif()

# Header-only libs
target_include_directories(${PROJECT_NAME} PRIVATE "contrib/include")
target_include_directories(${PROJECT_NAME} PRIVATE "contrib/dir_monitor/include")

#============================================================================
# Documentation generation
#============================================================================

if(BUILD_DOCUMENTATION)
	find_package(Doxygen)
	if(NOT DOXYGEN_FOUND)
		message(FATAL_ERROR
				"Doxygen is needed to build the documentation. Please install it correctly")
	endif()
	configure_file(Doxyfile.in ${PROJECT_BINARY_DIR}/Doxyfile @ONLY IMMEDIATE)
	add_custom_target(docs ALL COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile SOURCES ${PROJECT_BINARY_DIR}/Doxyfile)
endif()
