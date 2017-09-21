#============================================================================
# Internal compiler options
#============================================================================

set(CMAKE_INCLUDE_CURRENT_DIR ON)

#============================================================================
# Sources & headers
#============================================================================

file(GLOB_RECURSE metakit_SOURCES "*.cpp")
file(GLOB_RECURSE metakit_HEADERS "*.h")

#============================================================================
# Compile targets
#============================================================================

add_library(metakit STATIC ${meta_SOURCES} ${meta_HEADERS} ${PROTO_LIST})
target_include_directories(metakit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#============================================================================
# Third-party libraries
#============================================================================

target_link_libraries(metakit PRIVATE librevault-common)
target_link_libraries(metakit PRIVATE aes)
target_link_libraries(metakit PRIVATE secret)
target_link_libraries(metakit PUBLIC protobuf)
target_link_libraries(metakit PRIVATE Qt5::Core)
target_link_libraries(metakit PRIVATE cryptopp)
