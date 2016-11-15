#============================================================================
# Internal compiler options
#============================================================================
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_AUTOMOC ON)

#============================================================================
# Sources & headers
#============================================================================
# Universal sources
file(GLOB_RECURSE MAIN_SRCS "*.cpp")
file(GLOB_RECURSE MAIN_HEADERS "*.h")


list(APPEND SRCS ${MAIN_SRCS})
list(APPEND SRCS ${MAIN_HEADERS})

#============================================================================
# Compile targets
#============================================================================

add_executable(librevault ${SRCS})

#============================================================================
# Third-party libraries
#============================================================================

target_link_libraries(librevault lvcommon)
target_link_libraries(librevault openssl)
target_link_libraries(librevault docopt_s)

# Qt
target_link_libraries(librevault Qt5::Network)
