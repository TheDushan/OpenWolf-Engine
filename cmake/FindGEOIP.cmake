# GEOIP_LIBRARY
# GEOIP_FOUND

FIND_PATH(GEOIP_INCLUDE_DIR GeoIP
  /usr/local/Cellar/geoip/1.6.12
  /usr/local/Cellar/geoip
  ${LIB_DIR}/geoip/libGeoIP
)

message(STATUS "Found GEOIP include: ${GEOIP_INCLUDE_DIR}")

FIND_LIBRARY(GEOIP_LIBRARY
  NAMES libGeoIP libgeos
  HINTS
  $ENV{GEOIPDIR}
  /usr/local/Cellar/geoip/1.6.12
  ${LIB_DIR}/geoip/lib
  PATH_SUFFIXES lib64 lib)

  IF(GEOIP_LIBRARY)
    SET( GEOIP_LIBRARY ${GEOIP_LIBRARY} )
    SET( GEOIP_FOUND "YES" )
  ENDIF(GEOIP_LIBRARY)

message(STATUS "Found GEOIP library: ${GEOIP_LIBRARY}")

MARK_AS_ADVANCED(
  GEOIP_LIBRARY
  GEOIP_INCLUDE_DIR
)

#.rst:
# FindGeoIP
# ------------
#
# Locate GeoIP library
#
# This module defines
#
# ::
#
#   GEOIP_LIBRARIES, the library to link against
#   GEOIP_FOUND, if false, do not try to link to FREETYPE
#   GEOIP_INCLUDE_DIRS, where to find headers.
#   This is the concatenation of the paths:
#   GEOIP_INCLUDE_DIR
#
#=============================================================================
# Copyright 2014-2014 Martell Malone
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

FIND_PATH(GEOIP_INCLUDE_DIR GeoIP.h
  HINTS
  ENV GEOIP_DIR
  PATH_SUFFIXES include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
  ${LIB_DIR}/geoip/
  ${LIB_DIR}/geoip/include
  ${LIB_DIR}/geoip/libGeoIP
)

FIND_LIBRARY(GEOIP_LIBRARY 
  NAMES GeoIP libGeoIP libGeoIP_a
  HINTS
  ENV GEOIP_DIR
  PATH_SUFFIXES lib
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  ${LIB_DIR}/geoip/lib/lib64
)

set(GEOIP_INCLUDE_DIRS "${GEOIP_INCLUDE_DIR}")
set(GEOIP_LIBRARIES "${GEOIP_LIBRARY}")

#INCLUDE(${CMAKE_CURRENT_LIST_DIR}/FindPackageHandleStandardArgs.cmake)
# handle the QUIETLY and REQUIRED arguments and set GEOIP_FOUND to TRUE if 
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GeoIP DEFAULT_MSG GEOIP_LIBRARIES GEOIP_INCLUDE_DIR)

MARK_AS_ADVANCED(GEOIP_INCLUDE_DIR GEOIP_LIBRARIES GEOIP_LIBRARY)