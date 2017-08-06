#============================================================================
# Internal compiler options
#============================================================================

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

#============================================================================
# Sources & headers
#============================================================================

file(GLOB_RECURSE portmapping_SOURCES "*.cpp")
file(GLOB_RECURSE portmapping_HEADERS "*.h")

#============================================================================
# Compile targets
#============================================================================

add_library(portmapping STATIC ${portmapping_SOURCES} ${portmapping_HEADERS})
set_target_properties(portmapping PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(portmapping PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#============================================================================
# Third-party libraries
#============================================================================

target_link_libraries(portmapping PRIVATE natpmp)
target_link_libraries(portmapping PRIVATE libminiupnpc)

### WinSock
if(WIN32)
	target_link_libraries(portmapping PRIVATE ws2_32)
endif()

### Threads
target_link_libraries(portmapping PRIVATE Threads::Threads)

## Qt5
target_link_libraries(portmapping PRIVATE Qt5::Network)

### CryptoPP
target_link_libraries(portmapping PRIVATE cryptopp)
