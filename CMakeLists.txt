cmake_minimum_required(VERSION 3.2)
project(librevault-gui VERSION 0.9.0)

#============================================================================
# Settable options
#============================================================================

option(BUILD_STATIC "Build static version of binary" OFF)
option(BUILD_OPTWARE "Build software ready to be installed into /opt" OFF)

#============================================================================
# CMake modules
#============================================================================

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(GNUInstallDirs)

#============================================================================
# Internal compiler options
#============================================================================

if(BUILD_STATIC)
	set(CMAKE_CXX_FLAGS "-static-libgcc -static-libstdc++ ${CMAKE_CXX_FLAGS}")
	set(CMAKE_C_FLAGS "-static-libgcc -static-libstdc++ ${CMAKE_C_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ ${CMAKE_EXE_LINKER_FLAGS}")
	set(CMAKE_MODULE_LINKER_FLAGS "-static-libgcc -static-libstdc++ ${CMAKE_MODULE_LINKER_FLAGS}")
	set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()

# Setting compiler properties
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

#============================================================================
# Sources & headers
#============================================================================
list(APPEND SRCS "src/pch.h")

# Universal sources
file(GLOB_RECURSE MAIN_SRCS "src/*.cpp")
file(GLOB_RECURSE MAIN_HEADERS "src/*.h")
file(GLOB_RECURSE MAIN_UIS "src/*.ui")
file(GLOB_RECURSE MAIN_QRCS "resources/*.qrc")

# Windows sources
if(WIN32)
	configure_file(resources/librevault.rc librevault.rc @ONLY)
	configure_file(resources/librevault.ico librevault.ico COPYONLY)
	list(APPEND SRCS ${CMAKE_CURRENT_BINARY_DIR}/librevault.rc)
else()
	file(GLOB_RECURSE WIN_SRCS "src/*.win.cpp")
	list(REMOVE_ITEM MAIN_SRCS ${WIN_SRCS})
endif()

# Windows sources
if(NOT LINUX)
	file(GLOB_RECURSE LINUX_SRCS "src/*.linux.cpp")
	list(REMOVE_ITEM MAIN_SRCS ${LINUX_SRCS})
endif()

# OS X sources
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	file(GLOB_RECURSE MAIN_MMS "src/*.mm")
	list(APPEND SRCS ${MAIN_MMS})
else()
	file(GLOB_RECURSE MAC_SRCS "src/*.mac.cpp")
	list(REMOVE_ITEM MAIN_SRCS ${MAC_SRCS})
endif()

list(APPEND SRCS ${MAIN_SRCS})
list(APPEND SRCS ${MAIN_HEADERS})
list(APPEND SRCS ${MAIN_UIS})
list(APPEND SRCS ${MAIN_QRCS})

#============================================================================
# Third-party packages
#============================================================================
set(Qt_COMPONENTS
		LinguistTools
		Widgets
		WebSockets
		)
if(WIN32)
	list(APPEND Qt_COMPONENTS WinExtras)
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	list(APPEND Qt_COMPONENTS MacExtras)
endif()
find_package(Qt5 5.3 REQUIRED COMPONENTS ${Qt_COMPONENTS})
find_package(Threads REQUIRED)

# Translation
configure_file("src/lang/lang.qrc" "lang.qrc" COPYONLY)
qt5_add_translation(${PROJECT_NAME}_QM
		"src/lang/librevault_en.ts"
		"src/lang/librevault_ru.ts"
		)
qt5_add_resources(${PROJECT_NAME}_QM_RC ${CMAKE_BINARY_DIR}/lang.qrc)
list(APPEND SRCS ${${PROJECT_NAME}_QM_RC})

find_package(lvcommon)

#============================================================================
# Compile targets
#============================================================================
add_executable(${PROJECT_NAME} ${SRCS})

#============================================================================
# Third-party libraries
#============================================================================

# Qt
target_link_libraries(${PROJECT_NAME} Qt5::Widgets Qt5::WebSockets)
if(WIN32)
	target_link_libraries(${PROJECT_NAME} Qt5::WinExtras)
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	target_link_libraries(${PROJECT_NAME} Qt5::MacExtras)
endif()

# LVCommon
if(BUILD_STATIC)
	target_link_libraries(${PROJECT_NAME} lvcommon-static)
else()
	target_link_libraries(${PROJECT_NAME} lvcommon-shared)
endif()

## CoreFoundation
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	target_link_libraries(${PROJECT_NAME} "-framework CoreFoundation")
	target_link_libraries(${PROJECT_NAME} "-framework AppKit")
endif()

#============================================================================
# Install
#============================================================================
if(BUILD_OPTWARE)
	set_target_properties(${PROJECT_NAME} PROPERTIES
			INSTALL_RPATH "/opt/librevault/lib"
			BUILD_WITH_INSTALL_RPATH TRUE
			INSTALL_RPATH_USE_LINK_PATH TRUE
			)
	install(TARGETS ${PROJECT_NAME} DESTINATION "/opt/librevault")
else()
	install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
install(FILES "resources/Librevault.desktop" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES "resources/librevault_icon.svg" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps RENAME "librevault.svg")
