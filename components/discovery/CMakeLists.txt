#============================================================================
# Internal compiler options
#============================================================================

set(CMAKE_AUTOMOC ON)

#============================================================================
# Sources & headers
#============================================================================

file(GLOB_RECURSE discovery_SOURCES "library/*.cpp")
file(GLOB_RECURSE discovery_HEADERS "library/*.h")

file(GLOB_RECURSE discovery_testapp_SOURCES "testapp/*.cpp")
file(GLOB_RECURSE discovery_testapp_HEADERS "testapp/*.h")

#============================================================================
# Compile targets
#============================================================================

add_library(discovery STATIC ${discovery_SOURCES} ${discovery_HEADERS})
set_target_properties(discovery PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(discovery PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/library)

add_executable(discovery-testapp ${discovery_testapp_SOURCES} ${discovery_testapp_HEADERS})

#============================================================================
# Third-party libraries
#============================================================================

target_link_libraries(discovery PRIVATE dht)
target_link_libraries(discovery PRIVATE Threads::Threads)
target_link_libraries(discovery PUBLIC Qt5::Network)
if(WIN32)
	target_link_libraries(discovery PRIVATE ws2_32)
endif()

target_link_libraries(discovery-testapp PRIVATE discovery)
target_link_libraries(discovery-testapp PRIVATE docopt_s)
