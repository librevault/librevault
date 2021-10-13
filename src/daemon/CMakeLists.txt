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
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	file(GLOB_RECURSE WIN_SRCS "*.win.cpp")
	list(REMOVE_ITEM MAIN_SRCS ${WIN_SRCS})
endif()

# Linux sources
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	file(GLOB_RECURSE LINUX_SRCS "*.linux.cpp")
	list(REMOVE_ITEM MAIN_SRCS ${LINUX_SRCS})
endif()

# OS X sources
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
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
target_link_libraries(librevault-daemon PUBLIC dir_monitor)
target_link_libraries(librevault-daemon PUBLIC natpmp)
target_link_libraries(librevault-daemon PUBLIC dht)

##### External libraries #####

target_link_libraries(librevault-daemon PUBLIC Boost::system Boost::filesystem Boost::thread)
target_link_libraries(librevault-daemon PUBLIC Qt5::Xml Qt5::WebSockets)
target_link_libraries(librevault-daemon PUBLIC SQLite::SQLite3)
target_link_libraries(librevault-daemon PUBLIC websocketpp::websocketpp)
target_link_libraries(librevault-daemon PUBLIC docopt::docopt)

##### System libraries #####

## WinSock
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	target_link_libraries(librevault-daemon PUBLIC wsock32 ws2_32 Iphlpapi)
endif()

## CoreFoundation
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	target_link_libraries(librevault-daemon PUBLIC "-framework Foundation")
	target_link_libraries(librevault-daemon PUBLIC "-framework CoreFoundation")
	target_link_libraries(librevault-daemon PUBLIC "-framework CoreServices")
endif()
