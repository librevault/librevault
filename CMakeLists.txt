cmake_minimum_required(VERSION 3.2)
project(librevault)

#============================================================================
# Settable options
#============================================================================
option(USE_BUNDLED_SQLITE3 "Force using bundled version of SQLite3" OFF)

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

if(BUILD_STATIC)
	set(CMAKE_CXX_FLAGS "-static-libgcc -static-libstdc++ ${CMAKE_CXX_FLAGS}")
	set(CMAKE_C_FLAGS "-static-libgcc ${CMAKE_C_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ ${CMAKE_EXE_LINKER_FLAGS}")
	set(CMAKE_MODULE_LINKER_FLAGS "-static-libgcc -static-libstdc++ ${CMAKE_MODULE_LINKER_FLAGS}")
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".a .dll .so")
endif()

if(WIN32)
	add_definitions(-D_WIN32_WINNT=0x600)
	add_definitions(-DUNICODE)
endif()

## Calculating version
include(GetGitRevisionDescription)
git_describe(LV_APPVER)
configure_file("appver.in.h" "appver.h")
string(REGEX MATCH "^v(.+)\\.(.+)\\.(.+)-(.+)-(.+)$" VVV ${LV_APPVER})
message(${VVV})

string(REGEX REPLACE "^v([0-9]+)\\.[0-9]+\\.[0-9]+-?[0-9]*-?.*" "\\1" librevault_VERSION_MAJOR "${LV_APPVER}")
string(REGEX REPLACE "^v[0-9]+\\.([0-9]+)\\.[0-9]+-?[0-9]*-?.*" "\\1" librevault_VERSION_MINOR "${LV_APPVER}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+)-?[0-9]*-?.*" "\\1" librevault_VERSION_PATCH "${LV_APPVER}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+-?([0-9]*)-?.*" "\\1" librevault_VERSION_TWEAK "${LV_APPVER}")
if(librevault_VERSION_TWEAK)
	set(librevault_VERSION "${librevault_VERSION_MAJOR}.${librevault_VERSION_MINOR}.${librevault_VERSION_PATCH}.${librevault_VERSION_TWEAK}")
else()
	set(librevault_VERSION_TWEAK 0)
	set(librevault_VERSION "${librevault_VERSION_MAJOR}.${librevault_VERSION_MINOR}.${librevault_VERSION_PATCH}")
endif()

#============================================================================
# Third-party libraries
#============================================================================

##### System libraries #####

## Threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

##### Bundled libraries #####

## Dir_monitor
#find_package(dir_monitor QUIET)
#if(NOT dir_monitor_FOUND)
#	add_subdirectory("contrib/dir_monitor")
#endif()
add_library(dir_monitor INTERFACE)
target_include_directories(dir_monitor INTERFACE contrib/dir_monitor/include)

## Docopt
find_package(docopt QUIET)
if(NOT docopt_FOUND)
	add_subdirectory(contrib/docopt)
endif()

## JsonCpp
find_package(jsoncpp QUIET)
if(JSONCPP_FOUND)
	add_library(jsoncpp UNKNOWN IMPORTED)
	if(BUILD_STATIC)
		target_link_libraries(jsoncpp jsoncpp_lib_static)
	else()
		target_link_libraries(jsoncpp jsoncpp_lib)
	endif()
else()
	add_library(jsoncpp STATIC contrib/jsoncpp/jsoncpp.cpp)
	target_include_directories(jsoncpp PUBLIC "contrib/jsoncpp")
endif()

## libnatpmp
find_library(libnatpmp_LIBRARIES NAMES libnatpmp natpmp)
if(NOT libnatpmp_LIBRARIES)
	add_subdirectory(contrib/libnatpmp)
	target_link_libraries(${PROJECT_NAME} natpmp)
else()
	add_library(natpmp INTERFACE)
	target_link_libraries(natpmp INTERFACE ${libnatpmp_LIBRARIES})
endif()

## rabin
add_subdirectory("contrib/rabin")

## Spdlog
find_package(spdlog QUIET)
if(spdlog_FOUND)
	add_library(spdlog INTERFACE)
	target_link_libraries(spdlog INTERFACE spdlog::spdlog)
else()
	add_subdirectory(contrib/spdlog)
endif()

## SQLite
find_package(Sqlite3 QUIET)

if(SQLITE3_FOUND AND NOT USE_BUNDLED_SQLITE3)
	add_library(sqlite3 INTERFACE)
	target_include_directories(sqlite3 INTERFACE ${SQLITE3_INCLUDE_DIR})
	target_link_libraries(sqlite3 INTERFACE ${SQLITE3_LIBRARIES})
else()
	add_library(sqlite3 STATIC contrib/sqlite3/sqlite3.c)
	target_include_directories(sqlite3 PUBLIC "contrib/sqlite3")
endif()

## WebSocket++
find_package(websocketpp QUIET)
add_library(websocketpp INTERFACE)
if(NOT websocketpp_FOUND)
	add_subdirectory("contrib/websocketpp")
	target_include_directories(websocketpp INTERFACE "contrib/websocketpp")
endif()

##### External libraries #####

## Protobuf
find_package(Protobuf REQUIRED)
add_library(protobuf INTERFACE)
target_include_directories(protobuf INTERFACE ${PROTOBUF_INCLUDE_DIRS})
target_link_libraries(protobuf INTERFACE ${PROTOBUF_LIBRARIES})

# CryptoPP
find_package(CryptoPP 5.6.3 QUIET CONFIG)
add_library(cryptopp INTERFACE)
if(CryptoPP_FOUND)
	if(BUILD_STATIC)
		target_link_libraries(cryptopp INTERFACE cryptopp-static)
	else()
		target_link_libraries(cryptopp INTERFACE cryptopp-shared)
	endif()
else()
	find_package(CryptoPP 5.6.2 REQUIRED MODULE)

	target_include_directories(cryptopp INTERFACE ${CRYPTOPP_INCLUDE_DIRS})
	target_link_libraries(cryptopp INTERFACE ${CRYPTOPP_LIBRARIES})
endif()

## OpenSSL
find_package(OpenSSL REQUIRED)
add_library(openssl INTERFACE)
target_include_directories(openssl INTERFACE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(openssl INTERFACE ${OPENSSL_LIBRARIES})

#============================================================================
# Subprojects
#============================================================================

add_subdirectory("common")
add_subdirectory("daemon")
add_subdirectory("gui")

include(Install.cmake)