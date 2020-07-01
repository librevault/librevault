# OS detection flags
if(WIN32)
  set(OS_WIN TRUE)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(OS_LINUX TRUE)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(OS_MAC TRUE)
endif()

# Bitness detection
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(OS_64bit TRUE)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(OS_32bit TRUE)
endif()