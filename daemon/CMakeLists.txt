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
target_link_libraries(librevault-daemon QDocopt)
target_link_libraries(librevault-daemon secret)
target_link_libraries(librevault-daemon meta)
target_link_libraries(librevault-daemon aes)
target_link_libraries(librevault-daemon base32)
target_link_libraries(librevault-daemon protocol)
target_link_libraries(librevault-daemon portmapping)
target_link_libraries(librevault-daemon discovery)
target_link_libraries(librevault-daemon pathnormalizer)
target_link_libraries(librevault-daemon librevault-common)
target_link_libraries(librevault-daemon dir_monitor)
target_link_libraries(librevault-daemon rabin-cpp)
target_link_libraries(librevault-daemon sqlite3)
target_link_libraries(librevault-daemon Threads::Threads)
if(TARGET CONAN_PKG::spdlog)
	target_link_libraries(librevault-daemon CONAN_PKG::spdlog)
endif()

##### External libraries #####

target_link_libraries(librevault-daemon Boost::system Boost::filesystem Boost::thread)
target_link_libraries(librevault-daemon Qt5::Core Qt5::Network Qt5::WebSockets)
target_link_libraries(librevault-daemon cryptopp)
target_link_libraries(librevault-daemon OpenSSL::SSL)

##### System libraries #####

## WinSock
if(OS_WIN)
	target_link_libraries(librevault-daemon wsock32 ws2_32 Iphlpapi)
endif()

## CoreFoundation
if(OS_MAC)
	target_link_libraries(librevault-daemon "-framework Foundation")
	target_link_libraries(librevault-daemon "-framework CoreFoundation")
	target_link_libraries(librevault-daemon "-framework CoreServices")
endif()
