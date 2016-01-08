cmake_minimum_required(VERSION 3.4)
project(librevault-gui)

#============================================================================
# Internal compiler options
#============================================================================

add_subdirectory(liblibrevault)

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
# Compile targets
#============================================================================

add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME} liblibrevault)

#============================================================================
# Third-party libraries
#============================================================================

# Qt
find_package(Qt5Widgets)
find_package(Qt5WebSockets)

target_link_libraries(${PROJECT_NAME} Qt5::Widgets Qt5::WebSockets)

