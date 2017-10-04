#============================================================================
# Compile targets
#============================================================================

add_library(metakit STATIC metakit.cpp metakit.h)
target_include_directories(metakit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#============================================================================
# Third-party libraries
#============================================================================

target_link_libraries(metakit PUBLIC meta)
target_link_libraries(metakit PUBLIC Qt5::Core)
target_link_libraries(metakit PUBLIC Boost::filesystem)
