#============================================================================
# Internal compiler options
#============================================================================

set(CMAKE_AUTOMOC ON)

#============================================================================
# Sources & headers
#============================================================================

# Main sources
file(GLOB_RECURSE MAIN_SRCS "*.cpp")
file(GLOB_RECURSE MAIN_HEADERS "*.h" "*.hpp")

#============================================================================
# Compile targets
#============================================================================

add_library(librevault-common STATIC ${MAIN_SRCS} ${MAIN_HEADERS})
target_include_directories(librevault-common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#============================================================================
# Third-party libraries
#============================================================================

target_link_libraries(librevault-common PUBLIC Qt5::Core Qt5::Network)
target_link_libraries(librevault-common PUBLIC Threads::Threads)
if(WIN32)
    target_link_libraries(librevault-common PUBLIC ws2_32)
endif()
