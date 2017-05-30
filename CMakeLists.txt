cmake_minimum_required(VERSION 3.2)
project(librevault)

if(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
	include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	conan_basic_setup(TARGETS)
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
option(BUILD_CLI "Build CLI" ON)

# Parameters
option(BUILD_STATIC "Build static version of executable" OFF)
option(BUILD_UPDATER "Add auto-updater support" ON)
option(USE_BUNDLED_SQLITE3 "Force using bundled version of SQLite3" OFF)
option(USE_BUNDLED_MINIUPNP "Force using bundled version of miniupnp" OFF)
option(DEBUG_QT "Enable qDebug" OFF)
set(SANITIZE "none" CACHE STRING "What sanitizer to use. false for nothing")
option(INSTALL_BUNDLE "Prepare a bundle with all dependencies" OFF)

#============================================================================
# Internal compiler options
#============================================================================

# Setting compiler properties
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Error management
if(MSVC)
	# Force to always compile with W4
	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	# Update if necessary
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long")
endif()

# Sanitizer builds
if(NOT SANITIZE STREQUAL "none")
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

if(OS_WIN)
	add_definitions(-D_WIN32_WINNT=0x0601)
	add_definitions(-DUNICODE)
	add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

if(OS_MAC)
	add_definitions(-D__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=0)
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fobjc-arc")
	set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -fobjc-arc")
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

## Threads
add_library(threads INTERFACE)
if(OS_LINUX AND BUILD_STATIC)
	target_link_libraries(threads INTERFACE pthread)
	#target_compile_definitions(threads INTERFACE "-D_REENTRANT")
else()
	set(THREADS_PREFER_PTHREAD_FLAG ON)
	find_package(Threads)

	target_link_libraries(threads INTERFACE Threads::Threads)
endif()

##### Bundled libraries #####

## Dir_monitor
add_library(dir_monitor INTERFACE)
target_include_directories(dir_monitor INTERFACE contrib/dir_monitor/include)

## Docopt
find_package(docopt)

## libnatpmp
find_library(libnatpmp_LIBRARIES NAMES libnatpmp natpmp)
if(NOT libnatpmp_LIBRARIES)
	add_subdirectory(contrib/libnatpmp)
else()
	add_library(natpmp INTERFACE)
	target_link_libraries(natpmp INTERFACE ${libnatpmp_LIBRARIES})
endif()

## libminiupnpc
add_library(libminiupnpc INTERFACE)

if(NOT USE_BUNDLED_MINIUPNP)
	find_library(libminiupnpc_LIBRARIES NAMES libminiupnpc miniupnpc)
endif()

if(libminiupnpc_LIBRARIES AND NOT USE_BUNDLED_MINIUPNP)
	message(STATUS "Using system Miniupnp ${SQLITE3_VERSION}")
	target_link_libraries(libminiupnpc INTERFACE ${libminiupnpc_LIBRARIES})
else()
	message(STATUS "Using bundled Miniupnp")
	set(UPNPC_BUILD_SHARED FALSE)
	add_subdirectory(contrib/miniupnp/miniupnpc)
	target_link_libraries(libminiupnpc INTERFACE upnpc-static)
	target_include_directories(libminiupnpc INTERFACE contrib/miniupnp)
endif()

## rabin
add_subdirectory("contrib/rabin")

## dht
add_subdirectory("contrib/dht")

## QtSingleApplication
if(BUILD_GUI)
	add_subdirectory("contrib/qtsingleapplication")
endif()

## SQLite
if(NOT USE_BUNDLED_SQLITE3)
	find_package(Sqlite3 3.9 QUIET)
endif()

if(SQLITE3_FOUND AND NOT USE_BUNDLED_SQLITE3)
	message(STATUS "Using system SQLite ${SQLITE3_VERSION}")
	add_library(sqlite3 INTERFACE)
	target_include_directories(sqlite3 INTERFACE ${SQLITE3_INCLUDE_DIR})
	target_link_libraries(sqlite3 INTERFACE ${SQLITE3_LIBRARY})

	if(OS_LINUX)
		target_link_libraries(sqlite3 INTERFACE dl)
	endif()
else()
	message(STATUS "Using bundled SQLite")
	add_library(sqlite3 STATIC contrib/sqlite3/sqlite3.c)
	target_include_directories(sqlite3 PUBLIC "contrib/sqlite3")

	if(OS_LINUX)
		target_link_libraries(sqlite3 PUBLIC dl)
	endif()
endif()

##### External libraries #####

## Boost
if(BUILD_STATIC)
	set(Boost_USE_STATIC_LIBS ON)
endif()
find_package(Boost 1.58 REQUIRED COMPONENTS system filesystem thread)

## Qt5
set(Qt_COMPONENTS
		LinguistTools
#		Sql
		Svg
		Widgets
		WebSockets
		)
if(OS_WIN)
	list(APPEND Qt_COMPONENTS WinExtras)
elseif(OS_MAC)
	list(APPEND Qt_COMPONENTS MacExtras)
endif()
find_package(Qt5 5.7 REQUIRED COMPONENTS ${Qt_COMPONENTS})
add_definitions("-DQT_DISABLE_DEPRECATED_BEFORE=0x050700")

## Protobuf
find_package(Protobuf 3.0 REQUIRED)
add_library(protobuf INTERFACE)
target_include_directories(protobuf INTERFACE ${PROTOBUF_INCLUDE_DIRS})
target_link_libraries(protobuf INTERFACE ${PROTOBUF_LIBRARIES})

## CryptoPP
find_package(CryptoPP 5.6.4 REQUIRED MODULE)
add_library(cryptopp INTERFACE)
target_include_directories(cryptopp INTERFACE ${CRYPTOPP_INCLUDE_DIRS})
target_link_libraries(cryptopp INTERFACE ${CRYPTOPP_LIBRARIES})

## OpenSSL
find_package(OpenSSL 1.0.2 REQUIRED)

#============================================================================
# Subprojects
#============================================================================

add_subdirectory("common")
add_subdirectory("components/aes")
add_subdirectory("components/base32")
add_subdirectory("components/secret")
add_subdirectory("components/meta")
add_subdirectory("components/portmapping")
add_subdirectory("components/pathnormalizer")
add_subdirectory("components/discovery")
add_subdirectory("components/protocol")
if(BUILD_DAEMON)
	add_subdirectory("daemon")
endif()
if(BUILD_GUI)
	add_subdirectory("gui")
endif()
if(BUILD_CLI)
	add_subdirectory("cli")
endif()

include(Install.cmake)
