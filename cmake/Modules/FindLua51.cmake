# Locate Lua library
# This module defines
#  LUA_LIBRARIES
#  LUA_FOUND, if false, do not try to link to Lua 
#  LUA_INCLUDE_DIR, where to find lua.h 
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
  PATH_SUFFIXES include/lua51 include/lua5.1 include/lua include
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

FIND_LIBRARY(LUA_LIBRARY 
  NAMES lua51 lua5.1 lua lua-5.1
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

IF(LUA_LIBRARY)
  # include the math library for Unix
  IF(UNIX AND NOT APPLE)
    FIND_LIBRARY(LUA_MATH_LIBRARY m)
    SET( LUA_LIBRARIES "${LUA_LIBRARY};${LUA_MATH_LIBRARY}" CACHE STRING "Lua Libraries")
  # For Windows and Mac, don't need to explicitly include the math library
  ELSE(UNIX AND NOT APPLE)
    SET( LUA_LIBRARIES "${LUA_LIBRARY}" CACHE STRING "Lua Libraries")
  ENDIF(UNIX AND NOT APPLE)
ENDIF(LUA_LIBRARY)

# INCLUDE(FindPackageHandleStandardArgs)
# # handle the QUIETLY and REQUIRED arguments and set LUA_FOUND to TRUE if 
# # all listed variables are TRUE
# FIND_PACKAGE_HANDLE_STANDARD_ARGS(Lua  DEFAULT_MSG  LUA_LIBRARIES LUA_INCLUDE_DIR)
# MARK_AS_ADVANCED(LUA_INCLUDE_DIR LUA_LIBRARIES LUA_LIBRARY LUA_MATH_LIBRARY)

IF(LUA_INCLUDE_DIR AND LUA_LIBRARY)
  SET(LUA_FOUND 1)
  SET(LUA_VER 51)
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
