# - Find MySQL
# Find the MySQL includes and client library
# This module defines
#  MYSQL_INCLUDE_DIR, where to find mysql.h
#  MYSQL_LIBRARIES, the libraries needed to use MySQL.
#  MYSQL_FOUND, If false, do not try to use MySQL.
#
# Copyright (c) 2006, Jaroslaw Staniek, <js@iidea.pl>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
   set(MYSQL_FOUND TRUE)

else(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)

  find_path(MYSQL_INCLUDE_DIR mysql.h
      /usr/include/mysql
      /usr/local/include/mysql
      /usr/local/mysql-connector-c-6.1.11-macos10.12-x86_64/include
      $ENV{ProgramFiles}/MySQL/*/include
      $ENV{SystemDrive}/MySQL/*/include
      ${LIB_DIR}/mysql/include
      )

if(WIN32)
  find_library(MYSQL_LIBRARIES NAMES libmysql
      PATHS
      $ENV{ProgramFiles}/MySQL/*/lib/opt
      $ENV{SystemDrive}/MySQL/*/lib/opt
      ${LIB_DIR}/mysql/lib/win64
      )
else(WIN32)
  find_library(MYSQL_LIBRARIES NAMES mysqlclient libmysqlclient.18
      PATHS
      /usr/local/mysql-connector-c-6.1.11-macos10.12-x86_64/lib
      /${LIB_DIR}/mysql/lib/macos
      /usr/lib/mysql
      /usr/local/lib/mysql
      )
endif(WIN32)

  if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
    set(MYSQL_FOUND TRUE)
    message(STATUS "Found MySQL: ${MYSQL_INCLUDE_DIR}, ${MYSQL_LIBRARIES}")
  else(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
    set(MYSQL_FOUND FALSE)
    message(STATUS "MySQL not found.")
  endif(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)

  mark_as_advanced(MYSQL_INCLUDE_DIR MYSQL_LIBRARIES)

endif(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
