# Locate Lua library
# This module defines
#  LUA_LIBRARIES, both lua and lualib
#  LUA_FOUND, if false, do not try to link to Lua 
#  LUA_INCLUDE_DIR, where to find lua.h and lualib.h (and probably lauxlib.h)
#
# Note that the expected include convention is
#  #include "lua.h"
# and not
#  #include <lua/lua.h>
# This is because, the lua location is not standardized and may exist
# in locations other than lua/


FIND_PATH(LUA_INCLUDE_DIR lua.h
  HINTS
  $ENV{LUA_DIR}
  PATH_SUFFIXES include/lua50 include/lua5.0 include/lua5 include/lua include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/include
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

FIND_LIBRARY(LUA_LIBRARY_lua 
  NAMES lua50 lua5.0 lua5 lua lua-5.0
  HINTS
  $ENV{LUA_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)

# In an OS X framework, lualib is usually included as part of the framework
# (like GLU in OpenGL.framework)
IF(${LUA_LIBRARY_lua} MATCHES "framework")
  SET( LUA_LIBRARIES "${LUA_LIBRARY_lua}" CACHE STRING "Lua framework")
ELSE(${LUA_LIBRARY_lua} MATCHES "framework")

  FIND_LIBRARY(LUA_LIBRARY_lualib 
    NAMES lualib50 lualib5.0 lualib5 lualib
    HINTS
    $ENV{LUALIB_DIR}
    $ENV{LUA_DIR}
    PATH_SUFFIXES lib64 lib
    PATHS
    /usr/lib
    /usr/local
    /usr
    /sw
    /opt/local
    /opt/csw
    /opt
  )
  IF(LUA_LIBRARY_lualib AND LUA_LIBRARY_lua)
    # include the math library for Unix
    IF(UNIX AND NOT APPLE)
      FIND_LIBRARY(MATH_LIBRARY_FOR_LUA m)
      SET( LUA_LIBRARIES "${LUA_LIBRARY_lualib};${LUA_LIBRARY_lua};${MATH_LIBRARY_FOR_LUA}" CACHE STRING "This is the concatentation of lua and lualib libraries")
    # For Windows and Mac, don't need to explicitly include the math library
    ELSE(UNIX AND NOT APPLE)
      SET( LUA_LIBRARIES "${LUA_LIBRARY_lualib};${LUA_LIBRARY_lua}" CACHE STRING "This is the concatentation of lua and lualib libraries")
    ENDIF(UNIX AND NOT APPLE)
  ENDIF(LUA_LIBRARY_lualib AND LUA_LIBRARY_lua)
ENDIF(${LUA_LIBRARY_lua} MATCHES "framework")

# INCLUDE(FindPackageHandleStandardArgs)
# # handle the QUIETLY and REQUIRED arguments and set LUA_FOUND to TRUE if 
# # all listed variables are TRUE
# FIND_PACKAGE_HANDLE_STANDARD_ARGS(Lua  DEFAULT_MSG  LUA_LIBRARIES LUA_INCLUDE_DIR)
# MARK_AS_ADVANCED(LUA_INCLUDE_DIR LUA_LIBRARIES)


IF(LUA_INCLUDE_DIR AND LUA_LIBRARY)
  SET(LUA_FOUND 1)
  SET(LUA_VER 50)
ELSE(LUA_INCLUDE_DIR AND LUA_LIBRARY)
  SET(LUA_FOUND 0)
ENDIF(LUA_INCLUDE_DIR AND LUA_LIBRARY)

# Report the results.
IF(NOT LUA_FOUND)
  SET(LUA_DIR_MESSAGE
    "LUA was not found. Use LUALIB_DIR and LUA_DIR env variables.")
  IF(NOT LUA_FIND_QUIETLY)
    MESSAGE(STATUS "${LUA_DIR_MESSAGE}")
  ELSE(NOT LUA_FIND_QUIETLY)
    IF(LUA_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "${LUA_DIR_MESSAGE}")
    ENDIF(LUA_FIND_REQUIRED)
  ENDIF(NOT LUA_FIND_QUIETLY)
ENDIF(NOT LUA_FOUND)

