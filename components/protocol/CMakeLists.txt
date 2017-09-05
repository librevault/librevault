#============================================================================
# Internal compiler options
#============================================================================

set(CMAKE_INCLUDE_CURRENT_DIR ON)

#============================================================================
# Sources & headers
#============================================================================

file(GLOB_RECURSE protocol_SOURCES "*.cpp")
file(GLOB_RECURSE protocol_HEADERS "*.h")
file(GLOB_RECURSE PROTO_LIST "*.proto")
protobuf_generate_cpp(PROTO_SOURCES PROTO_HEADERS ${PROTO_LIST})
list(APPEND protocol_SOURCES ${PROTO_SOURCES})

#============================================================================
# Compile targets
#============================================================================

add_library(protocol STATIC ${protocol_SOURCES} ${protocol_HEADERS} ${PROTO_LIST})
target_include_directories(protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#============================================================================
# Third-party libraries
#============================================================================

target_link_libraries(protocol PRIVATE librevault-common)
target_link_libraries(protocol PRIVATE secret)
target_link_libraries(protocol PRIVATE meta)
target_link_libraries(protocol PUBLIC protobuf)
target_link_libraries(protocol PRIVATE Qt5::Core)
