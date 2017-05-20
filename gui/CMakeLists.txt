#============================================================================
# Internal compiler options
#============================================================================

if(WIN32)
	set(CMAKE_EXE_LINKER_FLAGS "-mwindows ${CMAKE_EXE_LINKER_FLAGS}")
	set(CMAKE_MODULE_LINKER_FLAGS "-mwindows ${CMAKE_MODULE_LINKER_FLAGS}")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

#============================================================================
# Sources & headers
#============================================================================
# Universal sources
file(GLOB_RECURSE MAIN_SRCS "*.cpp")
file(GLOB_RECURSE MAIN_HEADERS "*.h")
file(GLOB_RECURSE MAIN_UIS "*.ui")
file(GLOB_RECURSE MAIN_QRCS "resources/*.qrc")

# Windows sources
if(OS_WIN)
	configure_file(resources/librevault.rc librevault.rc @ONLY)
	configure_file(resources/librevault.ico librevault.ico COPYONLY)
	configure_file(resources/librevault.manifest librevault.manifest COPYONLY)
	list(APPEND SRCS ${CMAKE_CURRENT_BINARY_DIR}/librevault.rc)
else()
	file(GLOB_RECURSE WIN_SRCS "*.win.cpp")
	list(REMOVE_ITEM MAIN_SRCS ${WIN_SRCS})
endif()

# Linux sources
if(NOT OS_LINUX)
	file(GLOB_RECURSE LINUX_SRCS "*.linux.cpp")
	list(REMOVE_ITEM MAIN_SRCS ${LINUX_SRCS})
endif()

# OS X sources
if(OS_MAC)
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

# Translation
configure_file("lang/lang.qrc" "${CMAKE_CURRENT_BINARY_DIR}/lang.qrc" COPYONLY)
qt5_add_translation(librevault-gui_QM
		"lang/librevault_en.ts"
		"lang/librevault_ru.ts"
		)
qt5_add_resources(librevault-gui_QM_RC ${CMAKE_CURRENT_BINARY_DIR}/lang.qrc)
list(APPEND SRCS ${librevault-gui_QM_RC})

#============================================================================
# Compile targets
#============================================================================

add_executable(librevault-gui ${SRCS})

#============================================================================
# Target properties
#============================================================================

if(BUILD_UPDATER AND (OS_WIN OR OS_MAC))
	set(WILL_BUILD_UPDATER ON)
endif()

if(WILL_BUILD_UPDATER)
	target_compile_definitions(librevault-gui PRIVATE "BUILD_UPDATER")
endif()

#============================================================================
# Third-party libraries
#============================================================================

target_link_libraries(librevault-gui librevault-common)
target_link_libraries(librevault-gui secret)
target_link_libraries(librevault-gui OpenSSL::SSL OpenSSL::Crypto)

# Qt
target_link_libraries(librevault-gui Qt5::Svg Qt5::Widgets Qt5::WebSockets)
if(OS_WIN)
	target_link_libraries(librevault-gui Qt5::WinExtras)
endif()
if(OS_MAC)
	target_link_libraries(librevault-gui Qt5::MacExtras)
endif()

# QtSingleApplication
target_link_libraries(librevault-gui QtSingleApplication)

# WinSparkle
if(OS_WIN)
	set(SAVED_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
	set(CMAKE_FIND_LIBRARY_SUFFIXES .dll)
	find_library(winsparkle_LIBRARIES winsparkle)
	message(STATUS "WinSparkle library: ${winsparkle_LIBRARIES}")
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${SAVED_CMAKE_FIND_LIBRARY_SUFFIXES})
	target_link_libraries(librevault-gui ${winsparkle_LIBRARIES})
endif()

# Sparkle (for Macs)
if(OS_MAC)
	find_library(SPARKLE_LIBRARIES Sparkle)
	target_link_libraries(librevault-gui ${SPARKLE_LIBRARIES})
endif()

## CoreFoundation
if(OS_MAC)
	target_link_libraries(librevault-gui "-framework CoreFoundation")
	target_link_libraries(librevault-gui "-framework AppKit")
endif()
