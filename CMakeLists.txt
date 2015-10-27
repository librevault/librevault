project(librevault)
cmake_minimum_required(VERSION 3.1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(cotire)

#============================================================================
# Compilation options
#============================================================================

#============================================================================
# Compiler properties
#============================================================================

### Setting compiler properties
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

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
	#SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 --coverage")
endif()

if(CMAKE_COMPILER_IS_GNUCXX AND NOT WIN32)
	# Use -flto flag to enable GCC's link-time optimization.
	SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
	SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
	SET(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} -flto")
	if(MINGW)
		SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -fno-use-linker-plugin")
		SET(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} -fno-use-linker-plugin")
	ENDIF()
endif()

option(BUILD_STATIC "Build static version of executable" OFF)
if(BUILD_STATIC)
	SET(CMAKE_CXX_FLAGS "-static -static-libgcc -static-libstdc++ ${CMAKE_CXX_FLAGS}")
	SET(CMAKE_C_FLAGS "-static -static-libgcc -static-libstdc++ ${CMAKE_C_FLAGS}")
	SET(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++ ${CMAKE_EXE_LINKER_FLAGS}")
	SET(CMAKE_MODULE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++ ${CMAKE_MODULE_LINKER_FLAGS}")
	SET(Boost_USE_STATIC_LIBS ON)
	SET(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()

if (WIN32)
	add_definitions(-D_WIN32_WINNT=0x600)
	add_definitions(-DUNICODE)
endif()

#============================================================================
# Third-party Libraries
#============================================================================

# Cryptodiff
find_package(Cryptodiff REQUIRED)
include_directories(${CRYPTODIFF_INCLUDE_DIR})
link_libraries(${CRYPTODIFF_LIBRARIES})

# Pthreads
find_package(Threads)
link_libraries (${CMAKE_THREAD_LIBS_INIT})

# Boost
set(Boost_COMPONENTS
		system
		filesystem
		iostreams
		thread
		program_options)
# set (Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)

find_package(Boost REQUIRED COMPONENTS ${Boost_COMPONENTS})
include_directories(${Boost_INCLUDE_DIRS})
link_libraries(${Boost_LIBRARIES})
# /Boost

# SQLite
find_package(Sqlite3)

if(SQLITE3_FOUND)
	include_directories(${SQLITE3_INCLUDE_DIR})
	link_libraries(${SQLITE3_LIBRARIES})
else()
	include_directories("contrib/sqlite3")
endif()
# /SQLite

# Protobuf
find_package(Protobuf REQUIRED)

include_directories(${PROTOBUF_INCLUDE_DIRS})
link_libraries(${PROTOBUF_LIBRARIES})

file(GLOB_RECURSE PROTO_LIST "src/*.proto")
PROTOBUF_GENERATE_CPP(PROTO_SOURCES PROTO_HEADERS ${PROTO_LIST})

# CryptoPP
find_package(CryptoPP REQUIRED)
include_directories(${CRYPTOPP_INCLUDE_DIRS})
link_libraries(${CRYPTOPP_LIBRARIES})

# libnatpmp
find_library(libnatpmp_LIBRARIES natpmp)
link_libraries(${libnatpmp_LIBRARIES})

if (WIN32)
	add_definitions(-DNATPMP_STATICLIB)
endif()

# Botan
#find_package(Botan REQUIRED)
#include_directories(${BOTAN_INCLUDE_DIRS})
#link_libraries(${BOTAN_LIBRARIES})

# OpenSSL
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
link_libraries(${OPENSSL_LIBRARIES})

# WinSock
if (WIN32)
	link_libraries(wsock32)
	link_libraries(ws2_32)
	link_libraries(Iphlpapi)
endif()

if(BUILD_STATIC)
	if(UNIX)
		link_libraries(dl)
		link_libraries(c.a)
	endif()
	link_libraries(stdc++.a)
	link_libraries(m.a)
	link_libraries(pthread.a)
endif()

# Header-only libs
include_directories("contrib/include")
include_directories("contrib/dir_monitor/include")
include_directories("contrib/spdlog/include")

#============================================================================
# Source file configurarion
#============================================================================

include(GetGitRevisionDescription)
git_describe(LV_APPVER)
configure_file("appver.in.h" "appver.h")

#============================================================================
# Binary generation
#============================================================================

list(APPEND SRCS "src/pch.h")
# Protobuf
list(APPEND SRCS ${PROTO_SOURCES})
list(APPEND SRCS ${PROTO_HEADERS})
# SQLite
if(NOT SQLITE3_FOUND)
	list(APPEND SRCS "contrib/sqlite3/sqlite3.c")
endif()
file(GLOB_RECURSE LVCRYPTO_SRCS "contrib/include/lvcrypto/*")
list(APPEND SRCS ${LVCRYPTO_SRCS})
file(GLOB_RECURSE LVSQLITE3_SRCS "contrib/include/lvsqlite3/*")
list(APPEND SRCS ${LVSQLITE3_SRCS})

# Main sources
file(GLOB_RECURSE MAIN_SRCS "src/*")
list(APPEND SRCS ${MAIN_SRCS})

add_executable(${PROJECT_NAME} ${SRCS})

set_source_files_properties(${PROTO_SOURCES} PROPERTIES COTIRE_EXCLUDED ON)
cotire(${PROJECT_NAME})

#============================================================================
# Documentation generation
#============================================================================

#-- Add an Option to toggle the generation of the API documentation
option(BUILD_DOCUMENTATION "Use Doxygen to create the HTML based API documentation" OFF)
if(BUILD_DOCUMENTATION)
	FIND_PACKAGE(Doxygen)
	if(NOT DOXYGEN_FOUND)
		message(FATAL_ERROR
				"Doxygen is needed to build the documentation. Please install it correctly")
	endif()
	#-- Configure the Template Doxyfile for our specific project
	configure_file(Doxyfile.in ${PROJECT_BINARY_DIR}/Doxyfile @ONLY IMMEDIATE)
	#-- Add a custom target to run Doxygen when ever the project is built
	add_custom_target(docs ALL COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile SOURCES ${PROJECT_BINARY_DIR}/Doxyfile)
	# IF you do NOT want the documentation to be generated EVERY time you build the project
	# then leave out the 'ALL' keyword from the above command.
endif()
