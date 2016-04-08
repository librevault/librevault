cmake_minimum_required(VERSION 3.2)

#============================================================================
# Settable options
#============================================================================

option(BUILD_STATIC "Build static version of binary" OFF)
option(BUILD_OPTWARE "Build software ready to be installed into /opt" OFF)
option(BUILD_UPDATER "Add auto-updater support" ON)

#============================================================================
# CMake modules
#============================================================================

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(GNUInstallDirs)

#============================================================================
# Internal compiler options
#============================================================================

if(WIN32)
	set(CMAKE_EXE_LINKER_FLAGS "-mwindows ${CMAKE_EXE_LINKER_FLAGS}")
	set(CMAKE_MODULE_LINKER_FLAGS "-mwindows ${CMAKE_MODULE_LINKER_FLAGS}")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CMAKE_BINARY_DIR})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

#============================================================================
# Sources & headers
#============================================================================
list(APPEND SRCS "pch.h")

# Universal sources
file(GLOB_RECURSE MAIN_SRCS "*.cpp")
file(GLOB_RECURSE MAIN_HEADERS "*.h")
file(GLOB_RECURSE MAIN_UIS "*.ui")
file(GLOB_RECURSE MAIN_QRCS "resources/*.qrc")

# Windows sources
if(WIN32)
	configure_file(resources/librevault.rc librevault.rc @ONLY)
	configure_file(resources/librevault.ico librevault.ico COPYONLY)
	configure_file(resources/visualstyle.manifest visualstyle.manifest COPYONLY)
	list(APPEND SRCS ${CMAKE_CURRENT_BINARY_DIR}/librevault.rc)
else()
	file(GLOB_RECURSE WIN_SRCS "*.win.cpp")
	list(REMOVE_ITEM MAIN_SRCS ${WIN_SRCS})
endif()

# Linux sources
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	file(GLOB_RECURSE LINUX_SRCS "*.linux.cpp")
	list(REMOVE_ITEM MAIN_SRCS ${LINUX_SRCS})
endif()

# OS X sources
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	file(GLOB_RECURSE MAIN_MMS "*.mm")
	list(APPEND SRCS ${MAIN_MMS})
else()
	file(GLOB_RECURSE MAC_SRCS "*.mac.cpp")
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
		Svg
		Widgets
		WebSockets
		)
if(WIN32)
	list(APPEND Qt_COMPONENTS WinExtras)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	list(APPEND Qt_COMPONENTS MacExtras)
endif()
find_package(Qt5 5.3 REQUIRED COMPONENTS ${Qt_COMPONENTS})

# Translation
configure_file("lang/lang.qrc" "${CMAKE_CURRENT_BINARY_DIR}/lang.qrc" COPYONLY)
qt5_add_translation(librevault-gui_QM
		"lang/librevault_en.ts"
		"lang/librevault_ru.ts"
		)
message(${librevault-gui_QM})
qt5_add_resources(librevault-gui_QM_RC ${CMAKE_CURRENT_BINARY_DIR}/lang.qrc)
list(APPEND SRCS ${librevault-gui_QM_RC})

#============================================================================
# Compile targets
#============================================================================

add_executable(librevault-gui ${SRCS})

#============================================================================
# Target properties
#============================================================================

if(BUILD_UPDATER)
	target_compile_definitions(librevault-gui PRIVATE "BUILD_UPDATER")
endif()

#============================================================================
# Third-party libraries
#============================================================================

target_link_libraries(librevault-gui lvcommon-static)

# Qt
target_link_libraries(librevault-gui Qt5::Svg Qt5::Widgets Qt5::WebSockets)
if(WIN32)
	target_link_libraries(librevault-gui Qt5::WinExtras)
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	target_link_libraries(librevault-gui Qt5::MacExtras)
endif()

# LVCommon
if(BUILD_STATIC)
	target_link_libraries(librevault-gui lvcommon-static)
else()
	target_link_libraries(librevault-gui lvcommon-shared)
endif()

# WinSparkle
if(WIN32)
	set(SAVED_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
	set(CMAKE_FIND_LIBRARY_SUFFIXES .dll)
	find_library(winsparkle_LIBRARIES winsparkle)
	message("WinSparkle library: ${winsparkle_LIBRARIES}")
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${SAVED_CMAKE_FIND_LIBRARY_SUFFIXES})
	target_link_libraries(librevault-gui ${winsparkle_LIBRARIES})
endif()

# Sparkle (for Macs)
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	find_library(SPARKLE_LIBRARIES Sparkle)
	target_link_libraries(librevault-gui ${SPARKLE_LIBRARIES})
endif()

## CoreFoundation
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	target_link_libraries(librevault-gui "-framework CoreFoundation")
	target_link_libraries(librevault-gui "-framework AppKit")
endif()

#============================================================================
# Install
#============================================================================
if(BUILD_OPTWARE)
	set_target_properties(librevault-gui PROPERTIES
			INSTALL_RPATH "/opt/librevault/lib"
			BUILD_WITH_INSTALL_RPATH TRUE
			INSTALL_RPATH_USE_LINK_PATH TRUE
			)
	install(TARGETS librevault-gui DESTINATION "/opt/librevault")
else()
	install(TARGETS librevault-gui DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
install(FILES "resources/Librevault.desktop" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES "resources/librevault_icon.svg" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps RENAME "librevault.svg")
