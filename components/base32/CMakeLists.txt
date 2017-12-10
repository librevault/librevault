#============================================================================
# Internal compiler options
#============================================================================

set(CMAKE_INCLUDE_CURRENT_DIR ON)

#============================================================================
# Sources & headers
#============================================================================

file(GLOB_RECURSE base32_SOURCES "*.cpp")
file(GLOB_RECURSE base32_HEADERS "*.h")

#============================================================================
# Compile targets
#============================================================================

add_library(base32 STATIC ${base32_SOURCES} ${base32_HEADERS})
target_include_directories(base32 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#============================================================================
# Third-party libraries
#============================================================================

target_link_libraries(base32 PRIVATE Qt5::Core)
target_link_libraries(base32 PRIVATE cryptopp)
