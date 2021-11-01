cmake_minimum_required(VERSION 3.20)
project(librevault)

cmake_policy(SET CMP0071 NEW)

if(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
	include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	conan_basic_setup(TARGETS)
	list(APPEND CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}")
endif()

#============================================================================
# CMake modules
#============================================================================

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(GNUInstallDirs)
include(OSDetection)

#============================================================================
# Settable options
#============================================================================

# Components
option(BUILD_DAEMON "Build sync daemon" ON)
option(BUILD_GUI "Build GUI" ON)

# Parameters
option(USE_STATIC_BOOST "Use static version of boost" OFF)
option(BUILD_STATIC "Build static version of executable" OFF)
option(BUILD_UPDATER "Add auto-updater support" ON)
option(DEBUG_NORMALIZATION "Debug path normalization" OFF)
option(DEBUG_QT "Enable qDebug" OFF)
set(SANITIZE "false" CACHE STRING "What sanitizer to use. false for nothing")
option(INSTALL_BUNDLE "Prepare a bundle with all dependencies" OFF)

#============================================================================
# Internal compiler options
#============================================================================

# Setting compiler properties
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Error management
#add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/W4>)  # Maximum warnings
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)  # Multiprocess build
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/UTF-8>)  # Souce and execution encoding
#add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/showIncludes>)  # Useful for debug
add_compile_options($<$<CXX_COMPILER_ID:GNU>:-Wall>)
add_compile_options($<$<CXX_COMPILER_ID:GNU>:-Wpedantic>)

# Sanitizer builds
if(NOT SANITIZE STREQUAL "false")
	set(CMAKE_CXX_FLAGS "-fsanitize=${SANITIZE} ${CMAKE_CXX_FLAGS}")
	set(CMAKE_C_FLAGS "-fsanitize=${SANITIZE} ${CMAKE_C_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=${SANITIZE} ${CMAKE_EXE_LINKER_FLAGS}")
	set(CMAKE_MODULE_LINKER_FLAGS "-fsanitize=${SANITIZE} ${CMAKE_MODULE_LINKER_FLAGS}")
endif()

# Static builds
if(BUILD_STATIC)
	set(CMAKE_CXX_FLAGS "-static-libgcc -static-libstdc++ ${CMAKE_CXX_FLAGS}")
	set(CMAKE_C_FLAGS "-static-libgcc ${CMAKE_C_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ ${CMAKE_EXE_LINKER_FLAGS}")
	set(CMAKE_MODULE_LINKER_FLAGS "-static-libgcc -static-libstdc++ ${CMAKE_MODULE_LINKER_FLAGS}")
	set(CMAKE_FIND_LIBRARY_SUFFIXES .a .dll .so)
endif()

add_compile_definitions($<$<PLATFORM_ID:Windows>:_WIN32_WINNT=0x600>)
add_compile_definitions($<$<PLATFORM_ID:Windows>:UNICODE>)
add_compile_definitions($<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>)
add_compile_definitions($<$<PLATFORM_ID:Darwin>:__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=0>)
add_compile_options($<$<PLATFORM_ID:Darwin>:-fobjc-arc>)
add_link_options($<$<PLATFORM_ID:Darwin>:-fobjc-arc>)
add_link_options($<$<CXX_COMPILER_ID:MSVC>:/ignore:4099>)

if(DEBUG_NORMALIZATION)
	add_definitions(-DLV_DEBUG_NORMALIZATION)
endif()

if(NOT DEBUG_QT)
	add_definitions(-DQT_NO_DEBUG_OUTPUT)
endif()

## Calculating version
include(GetGitRevisionDescription)
git_describe(LV_APPVER)

if(LV_APPVER)
	set(LV_SOURCE "Git")
else()
	set(LV_SOURCE "Tar")
	file(READ "VERSION" LV_APPVER)
endif()

# Strip "v" before tag
string(SUBSTRING "${LV_APPVER}" 1 -1 LV_APPVER)

message(STATUS "Building Librevault ${LV_APPVER} from ${LV_SOURCE}")
configure_file("appver.in.h" "appver.h")

string(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+-?[0-9]*-?.*" "\\1" librevault_VERSION_MAJOR "${LV_APPVER}")
string(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\.[0-9]+-?[0-9]*-?.*" "\\1" librevault_VERSION_MINOR "${LV_APPVER}")
string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)-?[0-9]*-?.*" "\\1" librevault_VERSION_PATCH "${LV_APPVER}")
string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.[0-9]+-?([0-9]*)-?.*" "\\1" librevault_VERSION_TWEAK "${LV_APPVER}")
if(librevault_VERSION_TWEAK)
	set(librevault_VERSION "${librevault_VERSION_MAJOR}.${librevault_VERSION_MINOR}.${librevault_VERSION_PATCH}.${librevault_VERSION_TWEAK}")
else()
	set(librevault_VERSION_TWEAK 0)
	set(librevault_VERSION "${librevault_VERSION_MAJOR}.${librevault_VERSION_MINOR}.${librevault_VERSION_PATCH}")
endif()

#============================================================================
# Third-party libraries
#============================================================================

##### System libraries #####

##### External libraries #####

## Boost
if(BUILD_STATIC OR USE_STATIC_BOOST)
	set(Boost_USE_STATIC_LIBS ON)
endif()
add_definitions("-DBOOST_ASIO_DISABLE_CONCEPTS")
find_package(Boost REQUIRED COMPONENTS system filesystem thread)

# Qt5
if (CONAN_QT_ROOT)
	set (Qt5_DIR ${CONAN_QT_ROOT}/lib/cmake/Qt5) # to work in both multi and single build
else ()
	set (Qt5_DIR ${CONAN_QT_ROOT_RELEASE}/lib/cmake/Qt5) # to work in both multi and single build
endif ()

set(Qt_COMPONENTS
		LinguistTools
		Sql
		Svg
		Xml
		Widgets
		WebSockets
		)
if(OS_MAC)
	list(APPEND Qt_COMPONENTS MacExtras)
endif()
if(OS_WIN)
	list(APPEND Qt_COMPONENTS WinExtras)
endif()
find_package(Qt5 5.15 REQUIRED COMPONENTS ${Qt_COMPONENTS})
add_definitions("-DQT_DISABLE_DEPRECATED_BEFORE=0x050F00")
add_definitions("-DQT_NO_FOREACH=1")
add_definitions("-DQT_NO_FOREVER=1")

find_package(docopt REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Protobuf REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)
find_package(websocketpp REQUIRED)

##### Bundled libraries #####

## Dir_monitor
add_library(dir_monitor INTERFACE)
target_include_directories(dir_monitor INTERFACE contrib/dir_monitor/include)

add_subdirectory("contrib/libnatpmp")
add_subdirectory("contrib/dht")

# Corrosion
list(APPEND CORROSION_ENVIRONMENT_VARIABLES "OPENSSL_LIB_DIR=${OpenSSL_LIB_DIRS}")
list(APPEND CORROSION_ENVIRONMENT_VARIABLES "OPENSSL_INCLUDE_DIR=${OpenSSL_INCLUDE_DIR}")
add_subdirectory("contrib/corrosion")
corrosion_import_crate(MANIFEST_PATH Cargo.toml)
target_include_directories(librevault_util INTERFACE "${CMAKE_CURRENT_LIST_DIR}/src/util")
set_target_properties(daemon-rs PROPERTIES CORROSION_ENVIRONMENT_VARIABLES "${CORROSION_ENVIRONMENT_VARIABLES}")
set_target_properties(librevault_util PROPERTIES CORROSION_ENVIRONMENT_VARIABLES "${CORROSION_ENVIRONMENT_VARIABLES}")
set_target_properties(cli PROPERTIES CORROSION_ENVIRONMENT_VARIABLES "${CORROSION_ENVIRONMENT_VARIABLES}")
target_link_libraries(librevault_util INTERFACE OpenSSL::SSL OpenSSL::Crypto)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/contrib/corrosion/cmake")
find_package(Rust REQUIRED)

include(MakeCxxBridge)
add_cxxbridge(cxxbridge "src/util/cxx_bridges.txt" "librevault_util")

target_link_libraries(librevault_util INTERFACE cxxbridge)
target_link_libraries(cxxbridge PUBLIC librevault_util)

## QtSingleApplication
if(BUILD_GUI)
	add_subdirectory("contrib/qtsingleapplication")
endif()

#============================================================================
# Subprojects
#============================================================================

add_subdirectory("src/common")
if(BUILD_DAEMON)
	add_subdirectory("src/daemon")
endif()
if(BUILD_GUI)
	add_subdirectory("src/gui")
endif()

include(Install.cmake)
