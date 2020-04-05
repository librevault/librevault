#============================================================================
# Internal compiler options
#============================================================================

# Setting compiler properties
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

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
target_include_directories(lvcommon PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)

#============================================================================
# Third-party libraries
#============================================================================

### WinSock
if(WIN32)
	target_link_libraries(lvcommon PUBLIC ws2_32)
endif()

### Threads
target_link_libraries(lvcommon PUBLIC Threads::Threads)

### Protobuf
target_link_libraries(lvcommon PUBLIC Qt5::Core)
target_link_libraries(lvcommon PUBLIC CONAN_PKG::protobuf)
target_link_libraries(lvcommon PUBLIC CONAN_PKG::cryptopp)
