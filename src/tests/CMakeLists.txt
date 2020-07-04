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

list(APPEND SRCS ${MAIN_SRCS})
list(APPEND SRCS ${MAIN_HEADERS})

#============================================================================
# Compile targets
#============================================================================

add_executable(librevault-tests ${SRCS})

#============================================================================
# Third-party libraries
#============================================================================

##### Bundled libraries #####
target_link_libraries(librevault-tests PUBLIC librevault-common)
target_link_libraries(librevault-tests PUBLIC Qt5::Test)
