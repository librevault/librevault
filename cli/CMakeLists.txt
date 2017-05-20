#============================================================================
# Internal compiler options
#============================================================================
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

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

add_executable(librevault-cli ${SRCS})

#============================================================================
# Third-party libraries
#============================================================================

target_link_libraries(librevault-cli librevault-common)
target_link_libraries(librevault-cli secret)
target_link_libraries(librevault-cli OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(librevault-cli docopt_s)

# Qt
target_link_libraries(librevault-cli Qt5::Network Qt5::WebSockets)
