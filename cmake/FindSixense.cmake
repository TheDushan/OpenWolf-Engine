# 
#  FindSixense.cmake 
# 
#  Try to find the Sixense controller library
#
#  You must provide a SIXENSE_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  SIXENSE_FOUND - system found Sixense
#  SIXENSE_INCLUDE_DIRS - the Sixense include directory
#  SIXENSE_LIBRARIES - Link this to use Sixense
#  SIXENSE_LIBRARIES_UTILS -Link this to use Sixense Utils library
#

if (SIXENSE_LIBRARIES AND SIXENSE_INCLUDE_DIRS AND SIXENSE_LIBRARIES_UTILS)
  # in cache already
  set(SIXENSE_FOUND TRUE)
else ()
  
  set(SIXENSE_SEARCH_DIRS "${SIXENSE_ROOT_DIR}" "$ENV{HIFI_LIB_DIR}/sixense" ${LIB_DIR}/SixenseSDK_102215)
  
  find_path(SIXENSE_INCLUDE_DIRS sixense.h PATH_SUFFIXES include HINTS ${SIXENSE_SEARCH_DIRS})

  if (APPLE)
    find_library(SIXENSE_LIBRARIES lib/osx_x64/release_dll/libsixense_x64.dylib HINTS ${SIXENSE_SEARCH_DIRS})
  elseif (UNIX)
    find_library(SIXENSE_LIBRARIES lib/linux_x64/release/libsixense_x64.so HINTS ${SIXENSE_SEARCH_DIRS})
  elseif (WIN32)
    find_library(SIXENSE_LIBRARIES lib/x64/release_dll/sixense_x64.lib HINTS ${SIXENSE_SEARCH_DIRS})
  endif ()
  
  #Dushan
  if (APPLE)
    find_library(SIXENSE_LIBRARIES_UTILS lib/osx_x64/release_dll/libsixense_utils_x64.dylib HINTS ${SIXENSE_SEARCH_DIRS})
  elseif (UNIX)
    find_library(SIXENSE_LIBRARIES_UTILS lib/linux_x64/release/libsixense_utils_x64.so HINTS ${SIXENSE_SEARCH_DIRS})
  elseif (WIN32)
    find_library(SIXENSE_LIBRARIES_UTILS lib/x64/release_dll/sixense_utils_x64.lib HINTS ${SIXENSE_SEARCH_DIRS})
  endif ()

  if (SIXENSE_INCLUDE_DIRS AND SIXENSE_LIBRARIES AND SIXENSE_LIBRARIES_UTILS)
     set(SIXENSE_FOUND TRUE)
  endif ()
 
  if (SIXENSE_FOUND)
    if (NOT SIXENSE_FIND_QUIETLY)
      message(STATUS "Found Sixense: ${SIXENSE_LIBRARIES}")
    endif (NOT SIXENSE_FIND_QUIETLY)
  else ()
    if (SIXENSE_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find Sixense")
    endif (SIXENSE_FIND_REQUIRED)
  endif ()
endif ()

MESSAGE("SIXENSE_INCLUDE_DIRS is ${SIXENSE_INCLUDE_DIRS}")
MESSAGE("SIXENSE_LIBRARIES is ${SIXENSE_LIBRARIES}")
MESSAGE("SIXENSE_LIBRARIES_UTILS is ${SIXENSE_LIBRARIES_UTILS}")