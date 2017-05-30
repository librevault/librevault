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

file(GLOB_RECURSE secret_SOURCES "*.cpp")
file(GLOB_RECURSE secret_HEADERS "*.h")

#============================================================================
# Compile targets
#============================================================================

add_library(secret STATIC ${secret_SOURCES} ${secret_HEADERS})
target_include_directories(secret PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#============================================================================
# Third-party libraries
#============================================================================

target_link_libraries(secret PRIVATE Qt5::Core)
target_link_libraries(secret PRIVATE cryptopp)
