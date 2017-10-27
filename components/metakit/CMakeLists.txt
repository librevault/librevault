project(metakit)

#============================================================================
# Compile targets
#============================================================================

file(GLOB_RECURSE ${PROJECT_NAME}_SOURCES "*.cpp")
file(GLOB_RECURSE ${PROJECT_NAME}_HEADERS "*.h")

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SOURCES} ${${PROJECT_NAME}_HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#============================================================================
# Third-party libraries
#============================================================================

target_link_libraries(${PROJECT_NAME} PUBLIC meta)
target_link_libraries(${PROJECT_NAME} PUBLIC Qt5::Core)
target_link_libraries(${PROJECT_NAME} PUBLIC Boost::filesystem)
