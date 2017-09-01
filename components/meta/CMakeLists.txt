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

file(GLOB_RECURSE meta_SOURCES "*.cpp")
file(GLOB_RECURSE meta_HEADERS "*.h")
file(GLOB_RECURSE PROTO_LIST "*.proto")
protobuf_generate_cpp(PROTO_SOURCES PROTO_HEADERS ${PROTO_LIST})
list(APPEND meta_SOURCES ${PROTO_SOURCES})

#============================================================================
# Compile targets
#============================================================================

add_library(meta STATIC ${meta_SOURCES} ${meta_HEADERS} ${PROTO_LIST})
target_include_directories(meta PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#============================================================================
# Third-party libraries
#============================================================================

target_link_libraries(meta PRIVATE librevault-common)
target_link_libraries(meta PRIVATE aes)
target_link_libraries(meta PRIVATE secret)
target_link_libraries(meta PUBLIC protobuf)
target_link_libraries(meta PRIVATE Qt5::Core)
target_link_libraries(meta PRIVATE cryptopp)
