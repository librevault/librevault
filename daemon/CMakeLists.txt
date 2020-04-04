#============================================================================
# Internal compiler options
#============================================================================

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CMAKE_BINARY_DIR})

set(CMAKE_AUTOMOC ON)
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
target_link_libraries(librevault-daemon PRIVATE QDocopt)
target_link_libraries(librevault-daemon PRIVATE meta)
target_link_libraries(librevault-daemon PRIVATE metakit)
target_link_libraries(librevault-daemon PRIVATE protocol)
target_link_libraries(librevault-daemon PRIVATE portmapping)
target_link_libraries(librevault-daemon PRIVATE discovery)
target_link_libraries(librevault-daemon PRIVATE librevault-common)
target_link_libraries(librevault-daemon PRIVATE dir_monitor)
target_link_libraries(librevault-daemon PRIVATE rabin-cpp)
target_link_libraries(librevault-daemon PRIVATE Threads::Threads)
if(TARGET CONAN_PKG::sqlite3)
	target_link_libraries(librevault-daemon PRIVATE CONAN_PKG::sqlite3)
endif()
if(TARGET CONAN_PKG::spdlog)
	target_link_libraries(librevault-daemon PRIVATE CONAN_PKG::spdlog)
endif()

##### External libraries #####

target_link_libraries(librevault-daemon PRIVATE Boost::system Boost::filesystem Boost::thread)
target_link_libraries(librevault-daemon PRIVATE Qt5::Core Qt5::Network Qt5::WebSockets)
target_link_libraries(librevault-daemon PRIVATE OpenSSL::SSL)

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
