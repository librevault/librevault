#============================================================================
# Internal compiler options
#============================================================================

# Setting compiler properties
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#============================================================================
# Sources & headers
#============================================================================

file(GLOB_RECURSE lvcommon_SOURCES "src/*.cpp")
file(GLOB_RECURSE lvcommon_HEADERS "include/*.h")

#============================================================================
# Third-party packages
#============================================================================
file(GLOB_RECURSE PROTO_LIST "src/*.proto")
protobuf_generate_cpp(PROTO_SOURCES PROTO_HEADERS ${PROTO_LIST})
list(APPEND lvcommon_SOURCES ${PROTO_SOURCES})

#============================================================================
# Compile targets
#============================================================================

add_library(lvcommon STATIC ${lvcommon_SOURCES} ${lvcommon_HEADERS} ${PROTO_HEADERS})
set_target_properties(lvcommon PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(lvcommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

#============================================================================
# Third-party libraries
#============================================================================

target_link_libraries(lvcommon PRIVATE librevault-common)
target_link_libraries(lvcommon PUBLIC aes)
target_link_libraries(lvcommon PUBLIC secret)
target_link_libraries(lvcommon PUBLIC meta)

### WinSock
if(WIN32)
	target_link_libraries(lvcommon PUBLIC ws2_32)
endif()

### Threads
target_link_libraries(lvcommon PUBLIC Threads::Threads)

### Protobuf
target_link_libraries(lvcommon PUBLIC protobuf)

### CryptoPP
target_link_libraries(lvcommon PUBLIC cryptopp)
