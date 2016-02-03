cmake_minimum_required(VERSION 3.4)
project(lvcommon VERSION 0.9.1)

#============================================================================
# CMake modules
#============================================================================

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(GNUInstallDirs)

#============================================================================
# Internal compiler options
#============================================================================

# Setting compiler properties
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(WIN32)
	add_definitions(-D_WIN32_WINNT=0x600)
	add_definitions(-DUNICODE)
endif()

#============================================================================
# Sources & headers
#============================================================================

file(GLOB_RECURSE lvcommon_SOURCES "src/*.cpp")
file(GLOB_RECURSE lvcommon_HEADERS "include/*.h")

#============================================================================
# Third-party packages
#============================================================================
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Protobuf REQUIRED)
file(GLOB_RECURSE PROTO_LIST "src/*.proto")
protobuf_generate_cpp(PROTO_SOURCES PROTO_HEADERS ${PROTO_LIST})
list(APPEND lvcommon_SOURCES ${PROTO_SOURCES})

#============================================================================
# Compile targets
#============================================================================

add_library(lvcommon-object OBJECT ${lvcommon_SOURCES} ${PROTO_HEADERS})
set_target_properties(lvcommon-object PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(lvcommon-object PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)

add_library(lvcommon-static STATIC $<TARGET_OBJECTS:lvcommon-object>)
add_library(lvcommon-shared SHARED $<TARGET_OBJECTS:lvcommon-object>)

target_include_directories(lvcommon-static PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_include_directories(lvcommon-shared PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)

if(NOT MSVC)
	set(COMPAT_VERSION ${lvcommon_VERSION_MAJOR}.${lvcommon_VERSION_MINOR})

	set_target_properties(lvcommon-static PROPERTIES
			OUTPUT_NAME lvcommon
			)

	set_target_properties(lvcommon-shared PROPERTIES
			SOVERSION ${COMPAT_VERSION}
			OUTPUT_NAME lvcommon
			)
endif()

#============================================================================
# Third-party libraries
#============================================================================

### WinSock
if(WIN32)
	target_link_libraries(lvcommon-static ws2_32)
	target_link_libraries(lvcommon-shared ws2_32)
endif()

### Threads
target_link_libraries(lvcommon-static PRIVATE Threads::Threads)
target_link_libraries(lvcommon-shared PRIVATE Threads::Threads)

### Boost
set(Boost_COMPONENTS
		system
		filesystem
		iostreams)
set(Boost_USE_MULTITHREADED ON)

# shared executable
find_package(Boost COMPONENTS ${Boost_COMPONENTS} REQUIRED)
target_link_libraries(lvcommon-shared PRIVATE ${Boost_LIBRARIES})
target_include_directories(lvcommon-shared PRIVATE ${Boost_INCLUDE_DIRS})
message(STATUS "Boost shared libraries: ${Boost_LIBRARIES}")

# reset
unset(Boost_LIBRARIES)
set(Boost_USE_STATIC_LIBS ON)

# static executable
find_package(Boost COMPONENTS ${Boost_COMPONENTS} REQUIRED)
target_link_libraries(lvcommon-static PRIVATE ${Boost_LIBRARIES})
target_include_directories(lvcommon-static PRIVATE ${Boost_INCLUDE_DIRS})
message(STATUS "Boost static libraries: ${Boost_LIBRARIES}")

### Protobuf
target_include_directories(lvcommon-static PRIVATE ${PROTOBUF_INCLUDE_DIRS})
target_include_directories(lvcommon-shared PRIVATE ${PROTOBUF_INCLUDE_DIRS})

target_link_libraries(lvcommon-shared PRIVATE ${PROTOBUF_LIBRARIES})
target_link_libraries(lvcommon-static PRIVATE libprotobuf.a)

### CryptoPP
find_package(CryptoPP 5.6.3 REQUIRED)
target_link_libraries(lvcommon-static PRIVATE cryptopp-static)
target_link_libraries(lvcommon-shared PRIVATE cryptopp-shared)

### Cryptodiff
find_package(cryptodiff REQUIRED)
target_link_libraries(lvcommon-static PUBLIC cryptodiff-static)
target_link_libraries(lvcommon-shared PUBLIC cryptodiff-shared)

#============================================================================
# Install
#============================================================================
set(export_name "lvcommon-targets")

# Runtime package
install(TARGETS lvcommon-shared EXPORT ${export_name} DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Development package
install(TARGETS lvcommon-static EXPORT ${export_name} DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION include)

# Cmake Package
include(CMakePackageConfigHelpers)
write_basic_package_version_file("${PROJECT_BINARY_DIR}/lvcommon-config-version.cmake" COMPATIBILITY SameMajorVersion)
install(FILES cmake/lvcommon-config.cmake ${PROJECT_BINARY_DIR}/lvcommon-config-version.cmake DESTINATION "lib/cmake/lvcommon")
install(EXPORT ${export_name} DESTINATION "lib/cmake/lvcommon")
