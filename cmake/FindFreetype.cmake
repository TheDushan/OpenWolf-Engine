# - Find Freetype
# Find the Freetype includes and libraries
#
# Following variables are provided:
# FREETYPE_FOUND
#     True if Freetype has been found
# FREETYPE_INCLUDE_DIRS
#     The include directories of Freetype
# FREETYPE_LIBRARIES
#     Freetype library list

if(WIN32)
    find_path(FREETYPE_INCLUDE_DIRS NAMES freetype/freetype.h 
	PATHS 
	"${LIB_DIR}/freetype/include"
	"${LIB_DIR}/freetype/include/freetype2"
	"${LIB_DIR}/freetype"
	)
    find_library(FREETYPE_LIBRARY NAMES freetype PATHS "${LIB_DIR}/freetype/lib64")
    set(FREETYPE_FOUND 1)
    set(FREETYPE_LIBRARIES ${FREETYPE_LIBRARY})
elseif(APPLE)
    find_path(FREETYPE_INCLUDE_DIRS NAMES freetype/freetype.h PATHS "/Library/Frameworks/FreeType.Framework/Versions/2.4/unix/include")
    find_library(FREETYPE_LIBRARY NAMES freetype PATHS "/Library/Frameworks/FreeType.Framework/Versions/2.4/")
    include_directories(/Library/Frameworks/FreeType.Framework/Versions/2.4/unix/include)
    set(FREETYPE_FOUND 1)
    set(FREETYPE_LIBRARIES ${FREETYPE_LIBRARY})
elseif(UNIX)
    include(FindPkgConfig)
    pkg_check_modules(FREETYPE freetype2)
else()
    set(FREETYPE_FOUND 0)
endif()

