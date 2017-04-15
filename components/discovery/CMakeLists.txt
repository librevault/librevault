#============================================================================
# Internal compiler options
#============================================================================

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

#============================================================================
# Sources & headers
#============================================================================

file(GLOB_RECURSE multicast_SOURCES "library/multicast/*.cpp")
file(GLOB_RECURSE multicast_HEADERS "library/multicast/*.h")
file(GLOB_RECURSE mldht_SOURCES "library/mldht/*.cpp")
file(GLOB_RECURSE mldht_HEADERS "library/mldht/*.h")
file(GLOB discovery_SOURCES "library/*.cpp")
file(GLOB discovery_HEADERS "library/*.h")

file(GLOB_RECURSE discovery_testapp_SOURCES "testapp/*.cpp")
file(GLOB_RECURSE discovery_testapp_HEADERS "testapp/*.h")

#============================================================================
# Compile targets
#============================================================================

add_library(discovery STATIC ${discovery_SOURCES} ${discovery_HEADERS} ${multicast_SOURCES} ${multicast_HEADERS} ${mldht_SOURCES} ${mldht_HEADERS})
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
