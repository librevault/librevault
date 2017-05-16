find_package(CryptoPP 5.6.3 CONFIG QUIET)
if(NOT CryptoPP_FOUND)
    find_package(CryptoPP 5.6.2 REQUIRED MODULE)
endif()

include("${CMAKE_CURRENT_LIST_DIR}/lvcommon-targets.cmake")
