cmake_minimum_required(VERSION 3.16)
project(librevault)

if(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
	include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	conan_basic_setup(TARGETS KEEP_RPATHS)
endif()

#============================================================================
# CMake modules
#============================================================================

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(GNUInstallDirs)
include(OSDetection)
include(FetchContent)

#============================================================================
# Settable options
#============================================================================

# Components
option(BUILD_DAEMON "Build sync daemon" ON)
option(BUILD_GUI "Build GUI" ON)
option(BUILD_CLI "Build CLI" ON)

# Parameters
option(USE_STATIC_BOOST "Use static version of boost" OFF)
option(BUILD_STATIC "Build static version of executable" OFF)
option(BUILD_UPDATER "Add auto-updater support" ON)
option(DEBUG_NORMALIZATION "Debug path normalization" OFF)
option(DEBUG_QT "Enable qDebug" OFF)
set(SANITIZE "false" CACHE STRING "What sanitizer to use. false for nothing")
option(INSTALL_BUNDLE "Prepare a bundle with all dependencies" OFF)
option(INSTALL_RUNTIME "Install ld-linux.so" OFF)

#============================================================================
# Internal compiler options
#============================================================================

# Setting compiler properties
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Error management
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/W4>)  # Maximum warnings
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)  # Multiprocess build
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

if(DEBUG_NORMALIZATION)
	add_compile_definitions(LV_DEBUG_NORMALIZATION)
endif()

if(NOT DEBUG_QT)
	add_compile_definitions(QT_NO_DEBUG_OUTPUT)
endif()

## Calculating version
include(GetGitRevisionDescription)
file(READ "VERSION" LV_APPVER)
get_git_head_revision(LV_GIT_REF LV_GIT_HASH)

message(STATUS "Building Librevault ${LV_APPVER}")
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

## Threads
find_package(Threads)

##### External libraries #####

## Boost
if(BUILD_STATIC OR USE_STATIC_BOOST)
	set(Boost_USE_STATIC_LIBS ON)
endif()
add_compile_definitions("BOOST_ASIO_DISABLE_CONCEPTS")
find_package(Boost REQUIRED COMPONENTS system filesystem thread)

add_library(boost INTERFACE)
target_include_directories(boost INTERFACE ${Boost_INCLUDE_DIRS})
target_link_libraries(boost INTERFACE ${Boost_LIBRARIES})

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
find_package(Qt5 5.14 REQUIRED COMPONENTS ${Qt_COMPONENTS})
add_compile_definitions("QT_DISABLE_DEPRECATED_BEFORE=0x051400")
add_compile_definitions("QT_NO_FOREACH=1")
add_compile_definitions("QT_NO_FOREVER=1")

## Protobuf
find_package(Protobuf REQUIRED)

## OpenSSL
find_package(OpenSSL REQUIRED)
if(OS_WIN)
	target_link_libraries(OpenSSL::Crypto INTERFACE ws2_32)
elseif(OS_LINUX)
	target_link_libraries(OpenSSL::Crypto INTERFACE dl)
endif()

##### Bundled libraries #####

## libnatpmp
add_subdirectory("contrib/libnatpmp")

add_subdirectory("contrib/rabin")
add_subdirectory("contrib/dht")

## QtSingleApplication
if(BUILD_GUI)
	add_subdirectory("contrib/qtsingleapplication")
endif()

# Sparkle (for Macs)
if(OS_MAC)
	FetchContent_Declare(sparkle URL https://github.com/sparkle-project/Sparkle/releases/download/1.23.0/Sparkle-1.23.0.tar.xz)
	FetchContent_Populate(sparkle)
	find_library(SPARKLE_LIBRARIES Sparkle HINTS ${sparkle_SOURCE_DIR})
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
if(BUILD_CLI)
	add_subdirectory("src/cli")
endif()

include(Install.cmake)
