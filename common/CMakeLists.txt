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
file(GLOB_RECURSE MAIN_HEADERS "*.h" "*.hpp")
file(GLOB_RECURSE MAIN_QRCS "resources/*.qrc")

#============================================================================
# Compile targets
#============================================================================

add_library(librevault-common STATIC ${MAIN_SRCS} ${MAIN_HEADERS} ${MAIN_QRCS})
target_include_directories(librevault-common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#============================================================================
# Third-party libraries
#============================================================================

target_link_libraries(librevault-common Qt5::Core)
