#============================================================================
# Internal compiler options
#============================================================================

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CMAKE_BINARY_DIR})

#============================================================================
# Sources & headers
#============================================================================

# Main sources
file(GLOB_RECURSE MAIN_SRCS "*.cpp")
file(GLOB_RECURSE MAIN_HEADERS "*.h")

# Platform-dependent sources
if(NOT OS_WIN)
	file(GLOB_RECURSE WIN_SRCS "*.win.cpp")
	list(REMOVE_ITEM MAIN_SRCS ${WIN_SRCS})
endif()

# Linux sources
if(NOT OS_LINUX)
	file(GLOB_RECURSE LINUX_SRCS "*.linux.cpp")
	list(REMOVE_ITEM MAIN_SRCS ${LINUX_SRCS})
endif()

# FreeBSD sources
if(NOT OS_FREEBSD)
        file(GLOB_RECURSE FREEBSD_SRCS "*.bsd.cpp")
        list(REMOVE_ITEM MAIN_SRCS ${FREEBSD_SRCS})
endif()

# OS X sources
if(OS_MAC)
	file(GLOB_RECURSE MAIN_MMS "*.mm")
	list(APPEND SRCS ${MAIN_MMS})
else()
	#file(GLOB_RECURSE MAC_SRCS "*.mac.cpp")
	#list(REMOVE_ITEM MAIN_SRCS ${MAC_SRCS})
endif()

list(APPEND SRCS ${MAIN_SRCS})
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

## CryptoPP
target_link_libraries(librevault-daemon cryptopp)

## OpenSSL
target_link_libraries(librevault-daemon openssl)

## ICU
target_link_libraries(librevault-daemon icu)

##### System libraries #####

## Threads
target_link_libraries(librevault-daemon threads)

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

cotire(librevault-daemon)
