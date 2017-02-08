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
if(NOT PROTOBUF_FOUND)
	find_package(Protobuf REQUIRED)
endif()
file(GLOB_RECURSE PROTO_LIST "src/*.proto")
protobuf_generate_cpp(PROTO_SOURCES PROTO_HEADERS ${PROTO_LIST})
list(APPEND lvcommon_SOURCES ${PROTO_SOURCES})

#============================================================================
# Compile targets
#============================================================================

add_library(lvcommon STATIC ${lvcommon_SOURCES} ${lvcommon_HEADERS} ${PROTO_HEADERS})
set_target_properties(lvcommon PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(lvcommon PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)

if(NOT MSVC)
	set(COMPAT_VERSION ${librevault_VERSION_MAJOR}.${librevault_VERSION_MINOR}.${librevault_VERSION_PATCH})

	set_target_properties(lvcommon PROPERTIES
			SOVERSION ${COMPAT_VERSION}
			OUTPUT_NAME lvcommon
			)
endif()

#============================================================================
# Third-party libraries
#============================================================================

### WinSock
if(WIN32)
	target_link_libraries(lvcommon PUBLIC ws2_32)
endif()

### Threads
target_link_libraries(lvcommon PUBLIC threads)

### Protobuf
target_link_libraries(lvcommon PUBLIC protobuf)

### CryptoPP
target_link_libraries(lvcommon PUBLIC cryptopp)
