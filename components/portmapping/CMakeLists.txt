#============================================================================
# Internal compiler options
#============================================================================

set(CMAKE_AUTOMOC ON)

#============================================================================
# Sources & headers
#============================================================================

file(GLOB_RECURSE portmapping_SOURCES "library/*.cpp" "library/*.h")
#file(GLOB_RECURSE portmapping_testapp_SOURCES "testapp/*.cpp" "testapp/*.h")

#============================================================================
# Compile targets
#============================================================================

add_library(portmapping STATIC ${portmapping_SOURCES})
set_target_properties(portmapping PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(portmapping PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/library)

#add_executable(portmapping-testapp ${portmapping_testapp_SOURCES})

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
target_link_libraries(portmapping PUBLIC Qt5::Network)

#target_link_libraries(portmapping-testapp PRIVATE portmapping)
#target_link_libraries(portmapping-testapp PRIVATE QDocopt)
