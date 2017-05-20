#============================================================================
# Internal compiler options
#============================================================================

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

#============================================================================
# Sources & headers
#============================================================================

file(GLOB_RECURSE aes_SOURCES "*.cpp")
file(GLOB_RECURSE aes_HEADERS "*.h")

#============================================================================
# Compile targets
#============================================================================

add_library(aes STATIC ${aes_SOURCES} ${aes_HEADERS})
target_include_directories(aes PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#============================================================================
# Third-party libraries
#============================================================================

target_link_libraries(aes PRIVATE Qt5::Core)
target_link_libraries(aes PRIVATE cryptopp)
