#============================================================================
# Internal compiler options
#============================================================================

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CMAKE_BINARY_DIR})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

#============================================================================
# Sources & headers
#============================================================================

# Main sources
file(GLOB_RECURSE MAIN_SRCS "*.cpp")
file(GLOB_RECURSE MAIN_HEADERS "*.h")
file(GLOB_RECURSE MAIN_QRCS "resources/*.qrc")

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
list(APPEND SRCS ${MAIN_QRCS})

#============================================================================
# Compile targets
#============================================================================

add_executable(librevault-daemon ${SRCS})

#============================================================================
# Third-party libraries
#============================================================================

##### Bundled libraries #####
target_link_libraries(librevault-daemon PUBLIC librevault-common)
target_link_libraries(librevault-daemon PUBLIC natpmp)
target_link_libraries(librevault-daemon PUBLIC rabin)
target_link_libraries(librevault-daemon PUBLIC dht)

##### External libraries #####

## Boost
target_link_libraries(librevault-daemon PUBLIC boost)

## Qt5
target_link_libraries(librevault-daemon PUBLIC Qt5::Xml)
target_link_libraries(librevault-daemon PUBLIC Qt5::WebSockets)

## Protobuf
target_link_libraries(librevault-daemon PUBLIC OpenSSL::SSL)
target_link_libraries(librevault-daemon PUBLIC OpenSSL::Crypto)
target_link_libraries(librevault-daemon PUBLIC CONAN_PKG::spdlog)
target_link_libraries(librevault-daemon PUBLIC CONAN_PKG::sqlite3)
target_link_libraries(librevault-daemon PUBLIC CONAN_PKG::websocketpp)
target_link_libraries(librevault-daemon PUBLIC CONAN_PKG::cryptopp)
target_link_libraries(librevault-daemon PUBLIC CONAN_PKG::docopt.cpp)

##### System libraries #####

## WinSock
if(OS_WIN)
	target_link_libraries(librevault-daemon PUBLIC wsock32 ws2_32 Iphlpapi)
endif()

## CoreFoundation
if(OS_MAC)
	target_link_libraries(librevault-daemon PUBLIC "-framework Foundation")
	target_link_libraries(librevault-daemon PUBLIC "-framework CoreFoundation")
	target_link_libraries(librevault-daemon PUBLIC "-framework CoreServices")
endif()

if(BUILD_STATIC AND OS_LINUX)
	target_link_libraries(librevault-daemon PUBLIC dl)
endif()
