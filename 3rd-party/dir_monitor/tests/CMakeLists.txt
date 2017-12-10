include(CMakeParseArguments)

function(create_test NAME)
    cmake_parse_arguments(CT "NO_CTEST" "" "LIBS" ${ARGN})
    add_executable(test_${NAME} test_${NAME}.cpp directory.hpp check_paths.hpp)
    target_link_libraries(test_${NAME} ${CT_LIBS} ${Boost_LIBRARIES} dir_monitor)
    target_compile_definitions(test_${NAME} PRIVATE ${BOOST_TEST_LINK_MODE} BOOST_ASIO_ENABLE_HANDLER_TRACKING BOOST_ASIO_STANDALONE BOOST_ALL_NO_LIB BOOST_ASIO_HAS_BOOST_BIND)
    install(TARGETS test_${NAME}
        RUNTIME DESTINATION tests/unittests)
    if (NOT CT_NO_CTEST)
        add_test(${NAME} test_${NAME} --build_info=yes --log_level=all)
    endif (NOT CT_NO_CTEST)
endfunction(create_test)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_VERBOSE_MAKEFILE ON)

if (APPLE)
    find_library(COREFOUNDATION_LIB CoreFoundation)
    find_library(CORESERVICES_LIB CoreServices)
endif (APPLE)

if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g")
endif()

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DUNICODE /D_UNICODE /D_WINDOWS /W4 /D_SCL_SECURE_NO_WARNINGS /MP /Z7")
    add_definitions( -D_WIN32_WINNT=0x0600 ) # setting minimum supported target version to Vista
endif()

set(BOOST_TEST_LINK_MODE "BOOST_TEST_DYN_LINK")
if (BOOST_TEST_STATIC_LINK)
    set(BOOST_TEST_LINK_MODE "BOOST_TEST_NO_LIB")
endif()

create_test(sync LIBS ${Boost_LIBRARIES} ${COREFOUNDATION_LIB} ${CORESERVICES_LIB})
create_test(async LIBS ${Boost_LIBRARIES} ${COREFOUNDATION_LIB} ${CORESERVICES_LIB})
# TODO: this is not unit test module
#create_test(running LIBS ${Boost_LIBRARIES} ${COREFOUNDATION_LIB} ${CORESERVICES_LIB})
