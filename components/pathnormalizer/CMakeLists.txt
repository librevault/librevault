#============================================================================
# Compile targets
#============================================================================

add_library(pathnormalizer STATIC PathNormalizer.cpp PathNormalizer.h)
target_include_directories(pathnormalizer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#============================================================================
# Third-party libraries
#============================================================================

target_link_libraries(pathnormalizer PUBLIC Qt5::Core)
