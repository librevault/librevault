project(librevault)
cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

#============================================================================
# Compilation options
#============================================================================

#-- Add an Option to toggle the generation of the API documentation
option(SINGLE_THREADED "Build single-threaded version of the library" OFF)
if(SINGLE_THREADED)
	add_definitions(-DSINGLE_THREADED)
endif()

#============================================================================
# Compiler properties
#============================================================================

### Setting compiler properties
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC")
include_directories(${CMAKE_CURRENT_BINARY_DIR})
SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIC")

IF("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
# Use -flto flag to enable GCC's link-time optimization.
	SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -use-gold-plugin")
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -use-gold-plugin")
	SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -use-gold-plugin")
	SET(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} -use-gold-plugin")
ENDIF()

IF(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
# Use -flto flag to enable GCC's link-time optimization.
	SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
	SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
	SET(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} -flto")
ENDIF()

# Protobuf Generator
find_package(Protobuf REQUIRED)
if(PROTOBUF_FOUND)
  file(GLOB_RECURSE PROTO_COMMON_LIST "src/*.proto")
  PROTOBUF_GENERATE_CPP(PROTO_COMMON_SOURCES PROTO_COMMON_HEADERS ${PROTO_COMMON_LIST})
endif()

#============================================================================
# Binary generation
#============================================================================

file(GLOB_RECURSE SRCS "src/*")
# Protobuf
list(APPEND SRCS ${PROTO_COMMON_SOURCES})
list(APPEND SRCS ${PROTO_COMMON_HEADERS})
# SQLite
#list(APPEND SRCS "contrib/sqlite/sqlite3.c")
#list(APPEND SRCS "contrib/sqlite/sqlite3.h")
file(GLOB_RECURSE CRYPTO_SRCS "contrib/crypto/*")
list(APPEND SRCS ${CRYPTO_SRCS})
file(GLOB_RECURSE LVSQLITE3_SRCS "contrib/lvsqlite3/*")
list(APPEND SRCS ${LVSQLITE3_SRCS})

include_directories("src/include")

add_executable(${PROJECT_NAME} ${SRCS})

#============================================================================
# Third-party Libraries
#============================================================================

# Boost
set (Boost_COMPONENTS
	system
	filesystem
	date_time
	log
	program_options
	iostreams)

# set (Boost_USE_STATIC_LIBS ON)
set (Boost_USE_MULTITHREADED ON)

add_definitions(-DBOOST_LOG_DYN_LINK)
find_package(Boost COMPONENTS ${Boost_COMPONENTS} REQUIRED)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
endif()
# /Boost

# SQLite
find_package(Sqlite3 REQUIRED)

if(SQLITE3_FOUND)
  include_directories(${SQLITE3_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME} ${SQLITE3_LIBRARIES})
endif()
# /SQLite

# Protobuf
# Found lately

if(PROTOBUF_FOUND)
  include_directories(${PROTOBUF_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} ${PROTOBUF_LIBRARIES})
endif()
# /Protobuf

# SQLite
find_package(Cryptodiff REQUIRED)

if(CRYPTODIFF_FOUND)
  include_directories(${CRYPTODIFF_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME} ${CRYPTODIFF_LIBRARIES})
endif()
# /SQLite

# CryptoPP
find_package(CryptoPP REQUIRED)

if(CRYPTOPP_FOUND)
  include_directories(${CRYPTOPP_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} ${CRYPTOPP_LIBRARIES})
endif()
# /CryptoPP

#============================================================================
# Documentation generation
#============================================================================

#-- Add an Option to toggle the generation of the API documentation
option(BUILD_DOCUMENTATION "Use Doxygen to create the HTML based API documentation" OFF)
if(BUILD_DOCUMENTATION)
  FIND_PACKAGE(Doxygen)
  if (NOT DOXYGEN_FOUND)
    message(FATAL_ERROR
      "Doxygen is needed to build the documentation. Please install it correctly")
  endif()
  #-- Configure the Template Doxyfile for our specific project
  configure_file(Doxyfile.in ${PROJECT_BINARY_DIR}/Doxyfile @ONLY IMMEDIATE)
  #-- Add a custom target to run Doxygen when ever the project is built
  add_custom_target (docs ALL COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile SOURCES ${PROJECT_BINARY_DIR}/Doxyfile)
  # IF you do NOT want the documentation to be generated EVERY time you build the project
  # then leave out the 'ALL' keyword from the above command.
endif()
