cmake_policy(VERSION 3.16)

#============================================================================
# Internal compiler options
#============================================================================

# Setting compiler properties
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CMAKE_BINARY_DIR})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

#============================================================================
# Sources & headers
#============================================================================

file(GLOB_RECURSE MAIN_SRCS CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE MAIN_HEADERS CONFIGURE_DEPENDS "*.h")
file(GLOB_RECURSE MAIN_QRCS CONFIGURE_DEPENDS "resources/*.qrc")

# Platform-dependent sources
if(NOT OS_WIN)
	file(GLOB_RECURSE WIN_SRCS CONFIGURE_DEPENDS "*.win.cpp")
	list(REMOVE_ITEM MAIN_SRCS ${WIN_SRCS})
endif()

# Linux sources
if(NOT OS_LINUX)
	file(GLOB_RECURSE LINUX_SRCS CONFIGURE_DEPENDS "*.linux.cpp")
	list(REMOVE_ITEM MAIN_SRCS ${LINUX_SRCS})
endif()

# OS X sources
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	file(GLOB_RECURSE MAIN_MMS CONFIGURE_DEPENDS "*.mm")
	list(APPEND SRCS ${MAIN_MMS})
else()
	#file(GLOB_RECURSE MAC_SRCS CONFIGURE_DEPENDS "*.mac.cpp")
	#list(REMOVE_ITEM MAIN_SRCS ${MAC_SRCS})
endif()

#============================================================================
# Third-party packages
#============================================================================
if(NOT PROTOBUF_FOUND)
	find_package(Protobuf REQUIRED)
endif()
file(GLOB_RECURSE PROTO_LIST CONFIGURE_DEPENDS "*.proto")
protobuf_generate_cpp(PROTO_SOURCES PROTO_HEADERS ${PROTO_LIST})
list(APPEND MAIN_SRCS ${PROTO_SOURCES})
list(APPEND MAIN_HEADERS ${PROTO_HEADERS})

#============================================================================
# Compile targets
#============================================================================

add_library(librevault-common STATIC ${MAIN_SRCS} ${MAIN_HEADERS} ${MAIN_QRCS})
target_include_directories(librevault-common PUBLIC .)
target_include_directories(librevault-common PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/protobuf)

#============================================================================
# Third-party libraries
#============================================================================
target_link_libraries(librevault-common PUBLIC Threads::Threads)
target_link_libraries(librevault-common PUBLIC OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(librevault-common PUBLIC Qt5::Core Qt5::Network Qt5::Xml Qt5::WebSockets)
target_link_libraries(librevault-common PUBLIC Boost::system Boost::filesystem)
target_link_libraries(librevault-common PUBLIC protobuf::libprotobuf)
target_link_libraries(librevault-common PUBLIC CONAN_PKG::cryptopp)
target_link_libraries(librevault-common PUBLIC CONAN_PKG::magic_enum)
target_link_libraries(librevault-common PUBLIC CONAN_PKG::sqlite3)
target_link_libraries(librevault-common PUBLIC fmt::fmt)
target_link_libraries(librevault-common PUBLIC natpmp)
target_link_libraries(librevault-common PUBLIC dht)
target_link_libraries(librevault-common PUBLIC rabin)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	target_link_libraries(librevault-common PUBLIC wsock32 ws2_32 Iphlpapi)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	target_link_libraries(librevault-common PUBLIC "-framework Foundation")
	target_link_libraries(librevault-common PUBLIC "-framework CoreFoundation")
	target_link_libraries(librevault-common PUBLIC "-framework CoreServices")
endif()

if(BUILD_STATIC AND OS_LINUX)
	target_link_libraries(librevault-common PUBLIC dl)
endif()
