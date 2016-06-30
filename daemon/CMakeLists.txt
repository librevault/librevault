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

if(FILE_BACKEND STREQUAL "fopen")
	target_compile_definitions(librevault-daemon PRIVATE "-DFOPEN_BACKEND")
endif()

#============================================================================
# Third-party libraries
#============================================================================

##### Bundled libraries #####
target_link_libraries(librevault-daemon lvcommon)
target_link_libraries(librevault-daemon dir_monitor)
target_link_libraries(librevault-daemon spdlog)
target_link_libraries(librevault-daemon docopt_s)
target_link_libraries(librevault-daemon natpmp)
target_link_libraries(librevault-daemon libminiupnpc)
target_link_libraries(librevault-daemon rabin)
target_link_libraries(librevault-daemon dht)
target_link_libraries(librevault-daemon jsoncpp)
target_link_libraries(librevault-daemon sqlite3)
target_link_libraries(librevault-daemon websocketpp)

##### External libraries #####

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
