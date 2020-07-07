cmake_policy(VERSION 3.16)

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
file(GLOB_RECURSE MAIN_SRCS CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE MAIN_HEADERS CONFIGURE_DEPENDS "*.h")

# Platform-dependent sources
#if(NOT OS_WIN)
#	file(GLOB_RECURSE WIN_SRCS CONFIGURE_DEPENDS "*.win.cpp")
#	list(REMOVE_ITEM MAIN_SRCS ${WIN_SRCS})
#endif()

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
#	file(GLOB_RECURSE MAC_SRCS CONFIGURE_DEPENDS "*.mac.cpp")
#	list(REMOVE_ITEM MAIN_SRCS ${MAC_SRCS})
endif()

list(APPEND SRCS ${MAIN_SRCS})
list(APPEND SRCS ${MAIN_HEADERS})

#============================================================================
# Compile targets
#============================================================================

add_executable(librevault-daemon ${SRCS})

#============================================================================
# Third-party libraries
#============================================================================

##### Bundled libraries #####
target_link_libraries(librevault-daemon PUBLIC librevault-common)

##### External libraries #####

## Boost
target_link_libraries(librevault-daemon PUBLIC boost)

## Qt5
target_link_libraries(librevault-daemon PUBLIC Qt5::Core)
target_link_libraries(librevault-daemon PUBLIC OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(librevault-daemon PUBLIC CONAN_PKG::spdlog)
target_link_libraries(librevault-daemon PUBLIC CONAN_PKG::websocketpp)
target_link_libraries(librevault-daemon PUBLIC CONAN_PKG::cryptopp)
target_link_libraries(librevault-daemon PUBLIC CONAN_PKG::docopt.cpp)
