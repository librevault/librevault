cmake_policy(VERSION 3.16)

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

file(GLOB_RECURSE MAIN_SRCS CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE MAIN_HEADERS CONFIGURE_DEPENDS "*.h")

#============================================================================
# Third-party packages
#============================================================================
if(NOT PROTOBUF_FOUND)
	find_package(Protobuf REQUIRED)
endif()
file(GLOB_RECURSE PROTO_LIST CONFIGURE_DEPENDS "*.proto")
protobuf_generate_cpp(PROTO_SOURCES PROTO_HEADERS ${PROTO_LIST})
list(APPEND MAIN_SRCS ${PROTO_SOURCES})
list(APPEND MAIN_HEADERS ${PROTO_HEADERS})

#============================================================================
# Compile targets
#============================================================================

add_library(librevault-common STATIC ${MAIN_SRCS} ${MAIN_HEADERS})
target_include_directories(librevault-common PUBLIC .)

#============================================================================
# Third-party libraries
#============================================================================

### WinSock
if(WIN32)
	target_link_libraries(librevault-common PUBLIC ws2_32)
endif()

### Threads
target_link_libraries(librevault-common PUBLIC Threads::Threads)

### Protobuf
target_link_libraries(librevault-common PUBLIC Qt5::Core)
target_link_libraries(librevault-common PUBLIC Boost::system Boost::filesystem)
target_link_libraries(librevault-common PUBLIC CONAN_PKG::protobuf)
target_link_libraries(librevault-common PUBLIC CONAN_PKG::cryptopp)
target_link_libraries(librevault-common PUBLIC CONAN_PKG::magic_enum)
