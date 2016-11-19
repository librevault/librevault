cmake_minimum_required(VERSION 3.2)
project(QtSingleApplication)

#============================================================================
# Internal compiler options
#============================================================================

# Setting compiler properties
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 5.3 REQUIRED COMPONENTS Network Widgets)

#============================================================================
# Compile targets
#============================================================================
set(SRCS src/qtlocalpeer.cpp src/qtsingleapplication.cpp src/qtsinglecoreapplication.cpp)
add_library(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME} Qt5::Network Qt5::Widgets)

if(WIN32)
target_compile_definitions(${PROJECT_NAME} PRIVATE QT_QTSINGLEAPPLICATION_EXPORT)
target_compile_definitions(${PROJECT_NAME} INTERFACE QT_QTSINGLEAPPLICATION_IMPORT)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
