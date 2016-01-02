cmake_minimum_required(VERSION 3.4)
project(liblibrevault)

#============================================================================
# Settable options
#============================================================================

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

option(BUILD_STATIC "Build static version of executable" OFF)
if(BUILD_STATIC)
	set(CMAKE_CXX_FLAGS "-static -static-libgcc -static-libstdc++ ${CMAKE_CXX_FLAGS}")
	set(CMAKE_C_FLAGS "-static -static-libgcc -static-libstdc++ ${CMAKE_C_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++ ${CMAKE_EXE_LINKER_FLAGS}")
	set(CMAKE_MODULE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++ ${CMAKE_MODULE_LINKER_FLAGS}")
	set(Boost_USE_STATIC_LIBS ON)
	set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()

if(WIN32)
	add_definitions(-D_WIN32_WINNT=0x600)
	add_definitions(-DUNICODE)
endif()

#============================================================================
# Sources & headers
#============================================================================

# Main sources
file(GLOB_RECURSE MAIN_SRCS "src/*.cpp")
file(GLOB_RECURSE MAIN_HEADERS "src/*.h")
list(APPEND SRCS ${MAIN_SRCS})
list(APPEND SRCS ${MAIN_HEADERS})

#============================================================================
# Compile targets
#============================================================================

add_library(${PROJECT_NAME} ${SRCS})
target_include_directories(${PROJECT_NAME} PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
		)

if(NOT MSVC)
	set(COMPAT_VERSION ${cryptopp_VERSION_MAJOR}.${cryptopp_VERSION_MINOR})

	set_target_properties(liblibrevault
			PROPERTIES
			SOVERSION ${COMPAT_VERSION}
			OUTPUT_NAME librevault)
endif()

#============================================================================
# Third-party libraries
#============================================================================

# Cryptodiff
find_package(Cryptodiff REQUIRED)
target_link_libraries(${PROJECT_NAME} cryptodiff-shared)

# Pthreads
find_package(Threads)
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

# Boost
set(Boost_COMPONENTS
		system
		filesystem)
set(Boost_USE_MULTITHREADED ON)

find_package(Boost REQUIRED COMPONENTS ${Boost_COMPONENTS})
target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

# Protobuf
find_package(Protobuf REQUIRED)

file(GLOB_RECURSE PROTO_LIST "src/*.proto")
protobuf_generate_cpp(PROTO_SOURCES PROTO_HEADERS ${PROTO_LIST})

add_library(liblibrevault-protobuf STATIC ${PROTO_SOURCES} ${PROTO_HEADERS})
target_include_directories(liblibrevault-protobuf PUBLIC ${PROTOBUF_INCLUDE_DIRS})
target_link_libraries(liblibrevault-protobuf PUBLIC ${PROTOBUF_LIBRARIES})

target_link_libraries(${PROJECT_NAME} liblibrevault-protobuf)

# CryptoPP
find_package(CryptoPP 5.6.2 REQUIRED)
target_link_libraries(${PROJECT_NAME} cryptopp-shared)

#============================================================================
# Documentation generation
#============================================================================

if(BUILD_DOCUMENTATION)
	find_package(Doxygen)
	if(NOT DOXYGEN_FOUND)
		message(FATAL_ERROR
				"Doxygen is needed to build the documentation. Please install it correctly")
	endif()
	configure_file(Doxyfile.in ${PROJECT_BINARY_DIR}/Doxyfile @ONLY IMMEDIATE)
	add_custom_target(docs ALL COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile SOURCES ${PROJECT_BINARY_DIR}/Doxyfile)
endif()
