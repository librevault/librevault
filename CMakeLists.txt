cmake_minimum_required(VERSION 3.4)
project(librevault-gui)

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

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

#============================================================================
# Sources & headers
#============================================================================
list(APPEND SRCS "src/pch.h")
# Main sources
file(GLOB_RECURSE MAIN_SRCS "src/*.cpp")
file(GLOB_RECURSE MAIN_HEADERS "src/*.h")
file(GLOB_RECURSE MAIN_UIS "src/*.ui")
file(GLOB_RECURSE MAIN_QRCS "resources/*.qrc")
list(APPEND SRCS ${MAIN_SRCS})
list(APPEND SRCS ${MAIN_HEADERS})
list(APPEND SRCS ${MAIN_UIS})
list(APPEND SRCS ${MAIN_QRCS})

#============================================================================
# Third-party packages
#============================================================================
find_package(Qt5 5.4.0 REQUIRED COMPONENTS LinguistTools Widgets WebSockets WebKitWidgets)

# Translation
configure_file("src/lang/lang.qrc" "lang.qrc" COPYONLY)
qt5_add_translation(${PROJECT_NAME}_QM "src/lang/librevault_ru.ts")
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
target_link_libraries(${PROJECT_NAME} Qt5::Widgets Qt5::WebSockets Qt5::WebKitWidgets)

# LVCommon
target_link_libraries(${PROJECT_NAME} lvcommon-shared)

#============================================================================
# Third-party libraries
#============================================================================
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES "resources/Librevault.desktop" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps)
install(FILES "resources/librevault_icon.svg" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps RENAME "librevault.svg")
