cmake_minimum_required(VERSION 3.2)
project(librevault VERSION 0.1.0)

#============================================================================
# Settable options
#============================================================================
option(BUILD_STATIC "Build static version of executable" OFF)
option(WITH_COTIRE "Use cotire for builds" ON)
option(BUILD_DOCUMENTATION "Use Doxygen to create the HTML based API documentation" OFF)

#============================================================================
# CMake modules
#============================================================================

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
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

if(BUILD_STATIC)
	set(CMAKE_CXX_FLAGS "-static-libgcc -static-libstdc++ ${CMAKE_CXX_FLAGS}")
	set(CMAKE_C_FLAGS "-static-libgcc -static-libstdc++ ${CMAKE_C_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ ${CMAKE_EXE_LINKER_FLAGS}")
	set(CMAKE_MODULE_LINKER_FLAGS "-static-libgcc -static-libstdc++ ${CMAKE_MODULE_LINKER_FLAGS}")
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

#============================================================================
# Third-party libraries
#============================================================================

# LVCommon
find_package(lvcommon)
if(BUILD_STATIC)
	target_link_libraries(${PROJECT_NAME} lvcommon-static)
else()
	target_link_libraries(${PROJECT_NAME} lvcommon-shared)
endif()

##### Bundled libraries #####
## Dir_monitor
#find_package(dir_monitor QUIET)
#if(NOT dir_monitor_FOUND)
#	add_subdirectory("contrib/dir_monitor")
#endif()
#target_link_libraries(${PROJECT_NAME} dir_monitor)
target_include_directories(${PROJECT_NAME} PUBLIC contrib/dir_monitor/include)

## Docopt
find_package(docopt QUIET)
if(NOT docopt_FOUND)
	add_subdirectory(contrib/docopt)
endif()
target_link_libraries(${PROJECT_NAME} docopt_s)

## JsonCpp
find_package(jsoncpp QUIET)

if(JSONCPP_FOUND)
	if(BUILD_STATIC)
		target_link_libraries(${PROJECT_NAME} jsoncpp_lib)
	else()
		target_link_libraries(${PROJECT_NAME} jsoncpp_lib_static)
	endif()
else()
	add_library(jsoncpp-bundled STATIC contrib/jsoncpp/jsoncpp.cpp)
	target_include_directories(jsoncpp-bundled PUBLIC "contrib/jsoncpp")
	target_link_libraries(${PROJECT_NAME} jsoncpp-bundled)
endif()

target_link_libraries(${PROJECT_NAME} jsoncpp-bundled)

## libnatpmp
find_library(libnatpmp_LIBRARIES libnatpmp)
if(NOT libnatpmp_FOUND)
	add_subdirectory(contrib/libnatpmp)
	target_link_libraries(${PROJECT_NAME} natpmp)
else()
	target_link_libraries(${PROJECT_NAME} ${libnatpmp_LIBRARIES})
endif()

## rabin
add_subdirectory("contrib/rabin")
target_link_libraries(${PROJECT_NAME} rabin)

## Spdlog
find_package(spdlog QUIET)
if(spdlog_FOUND)
	target_link_libraries(${PROJECT_NAME} spdlog::spdlog)
else()
	add_subdirectory(contrib/spdlog)
	target_link_libraries(${PROJECT_NAME} spdlog)
endif()

## SQLite
find_package(Sqlite3 QUIET)

if(SQLITE3_FOUND)
	target_include_directories(${PROJECT_NAME} PUBLIC ${SQLITE3_INCLUDE_DIR})
	target_link_libraries(${PROJECT_NAME} ${SQLITE3_LIBRARIES})
else()
	add_library(sqlite3-bundled STATIC contrib/sqlite3/sqlite3.c)
	target_include_directories(sqlite3-bundled PUBLIC "contrib/sqlite3")
	target_link_libraries(${PROJECT_NAME} sqlite3-bundled)
endif()

## WebSocket++
find_package(websocketpp QUIET)
if(NOT websocketpp_FOUND)
	add_subdirectory("contrib/websocketpp")
	target_include_directories(${PROJECT_NAME} PUBLIC "contrib/websocketpp")
endif()

##### External libraries #####

# Boost
set(Boost_COMPONENTS
		system
		filesystem
		iostreams
		thread)
if(BUILD_STATIC)
	set(Boost_USE_STATIC_LIBS ON)
endif()

find_package(Boost REQUIRED COMPONENTS ${Boost_COMPONENTS})
target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

## Protobuf
find_package(Protobuf REQUIRED)

file(GLOB_RECURSE PROTO_LIST "src/*.proto")
protobuf_generate_cpp(PROTO_SOURCES PROTO_HEADERS ${PROTO_LIST})

add_library(librevault-protobuf STATIC ${PROTO_SOURCES} ${PROTO_HEADERS})
target_include_directories(librevault-protobuf PUBLIC ${PROTOBUF_INCLUDE_DIRS})
target_link_libraries(librevault-protobuf PUBLIC ${PROTOBUF_LIBRARIES})

target_link_libraries(${PROJECT_NAME} librevault-protobuf)

# CryptoPP
find_package(CryptoPP 5.6.3 CONFIG)
if(CryptoPP_FOUND)
	if(BUILD_STATIC)
		target_link_libraries(${PROJECT_NAME} cryptopp-static)
	else()
		target_link_libraries(${PROJECT_NAME} cryptopp-shared)
	endif()
else()
	find_package(CryptoPP 5.6.2 REQUIRED MODULE)

	target_include_directories(${PROJECT_NAME} PUBLIC ${CRYPTOPP_INCLUDE_DIRS})
	target_link_libraries(${PROJECT_NAME} ${CRYPTOPP_LIBRARIES})
endif()

## OpenSSL
find_package(OpenSSL REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})

##### System libraries #####

## Threads
find_package(Threads)
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

## WinSock
if(WIN32)
	target_link_libraries(${PROJECT_NAME} wsock32)
	target_link_libraries(${PROJECT_NAME} ws2_32)
	target_link_libraries(${PROJECT_NAME} Iphlpapi)
endif()

if(BUILD_STATIC)
#	target_link_libraries(${PROJECT_NAME} stdc++.a)
#	target_link_libraries(${PROJECT_NAME} m.a)
	if(UNIX)
		target_link_libraries(${PROJECT_NAME} dl)
#		target_link_libraries(${PROJECT_NAME} c.a)
	endif()
endif()

#============================================================================
# Documentation generation
#============================================================================

if(BUILD_DOCUMENTATION)
	find_package(Doxygen REQUIRED)
	configure_file(Doxyfile.in ${PROJECT_BINARY_DIR}/Doxyfile @ONLY IMMEDIATE)
	add_custom_target(docs ALL COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile SOURCES ${PROJECT_BINARY_DIR}/Doxyfile)
endif()

#============================================================================
# Install
#============================================================================
# Development package
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
