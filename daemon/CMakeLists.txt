cmake_minimum_required(VERSION 3.2)

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

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CMAKE_BINARY_DIR})

#============================================================================
# Sources & headers
#============================================================================

list(APPEND SRCS "pch.h")
# Main sources
file(GLOB_RECURSE MAIN_CXX "*.cpp")
file(GLOB_RECURSE MAIN_HEADERS "*.h")
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	file(GLOB_RECURSE MAIN_OBJCXX "*.mm")
endif()

list(APPEND SRCS ${MAIN_CXX})
list(APPEND SRCS ${MAIN_HEADERS})
list(APPEND SRCS ${MAIN_OBJCXX})

#============================================================================
# Compile targets
#============================================================================

add_executable(librevault-daemon ${SRCS})

#============================================================================
# Third-party libraries
#============================================================================

##### Bundled libraries #####
target_link_libraries(librevault-daemon lvcommon-static)
target_link_libraries(librevault-daemon dir_monitor)
target_link_libraries(librevault-daemon spdlog)
target_link_libraries(librevault-daemon docopt_s)
target_link_libraries(librevault-daemon natpmp)
target_link_libraries(librevault-daemon rabin)
target_link_libraries(librevault-daemon jsoncpp)
target_link_libraries(librevault-daemon sqlite3)
target_link_libraries(librevault-daemon websocketpp)

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
target_include_directories(librevault-daemon PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(librevault-daemon ${Boost_LIBRARIES})

## Protobuf
file(GLOB_RECURSE PROTO_LIST "*.proto")
protobuf_generate_cpp(PROTO_SOURCES PROTO_HEADERS ${PROTO_LIST})

add_library(librevault-protobuf STATIC ${PROTO_SOURCES} ${PROTO_HEADERS})
target_link_libraries(librevault-protobuf PUBLIC protobuf)

target_link_libraries(librevault-daemon librevault-protobuf)

# CryptoPP
target_link_libraries(librevault-daemon cryptopp)

## OpenSSL
target_link_libraries(librevault-daemon openssl)

##### System libraries #####

## Threads
target_link_libraries(librevault-daemon Threads::Threads)

## WinSock
if(WIN32)
	target_link_libraries(librevault-daemon wsock32 ws2_32 Iphlpapi)
endif()

## CoreFoundation
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	target_link_libraries(librevault-daemon "-framework Foundation")
	target_link_libraries(librevault-daemon "-framework CoreFoundation")
	target_link_libraries(librevault-daemon "-framework CoreServices")
endif()

if(BUILD_STATIC)
#	target_link_libraries(librevault-daemon stdc++.a)
#	target_link_libraries(librevault-daemon m.a)
	if(UNIX)
		target_link_libraries(librevault-daemon dl)
#		target_link_libraries(librevault-daemon c.a)
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
install(TARGETS librevault-daemon DESTINATION ${CMAKE_INSTALL_BINDIR})
