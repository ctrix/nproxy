
PROJECT(NPROXY C)

SET(APPLICATION_NAME "nproxy")
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.7)


if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy(SET CMP0005 NEW)
endif(COMMAND cmake_policy)

set(CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/cmake/Modules
)

#*************************************************
# The following checks are distributed with CMake
#*************************************************
INCLUDE(CheckCSourceCompiles)
INCLUDE(CheckCCompilerFlag)
INCLUDE(CheckIncludeFile)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckSymbolExists)
INCLUDE(CheckTypeSize)
INCLUDE(CheckLibraryExists)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckLibraryExists)
INCLUDE(CheckIncludeFile)
INCLUDE(TestBigEndian)

#*************************
# My checks
#*************************
INCLUDE(DefineCMakeDefaults)
INCLUDE(DefineCompilerFlags)
INCLUDE(DefineInstallationPaths)
INCLUDE(FunctionsChecks)
INCLUDE(HeadersChecks)
INCLUDE(SignalsChecks)
INCLUDE(TestInline)

#****************************
# NProxy binary default data
#****************************

MARK_AS_ADVANCED( NPROXY_USER )
IF(NOT NPROXY_USER)
    SET(NPROXY_USER  "nproxy")
ENDIF(NOT NPROXY_USER)

MARK_AS_ADVANCED( NPROXY_GROUP )
IF(NOT NPROXY_GROUP)
    SET(NPROXY_GROUP "nproxy")
ENDIF(NOT NPROXY_GROUP)

MARK_AS_ADVANCED( NPROXY_STATE_DIR )
IF(NOT NPROXY_STATE_DIR)
    SET(NPROXY_STATE_DIR ${STATE_INSTALL_DIR})
ENDIF(NOT NPROXY_STATE_DIR)

MARK_AS_ADVANCED( NPROXY_LOG_DIR )
IF(NOT NPROXY_LOG_DIR)
    SET(NPROXY_LOG_DIR ${LOG_INSTALL_DIR})
ENDIF(NOT NPROXY_LOG_DIR)

MARK_AS_ADVANCED( NPROXY_CONFIG_DIR )
IF( NOT NPROXY_CONFIG_DIR )
    SET(NPROXY_CONFIG_DIR ${SYSCONF_INSTALL_DIR})
ENDIF(NOT NPROXY_CONFIG_DIR )

# TODO (only when debugging
IF(CMAKE_COMPILER_IS_GNUCXX )
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wstrict-prototypes -fomit-frame-pointer -O0 -g -ggdb")
ENDIF(CMAKE_COMPILER_IS_GNUCXX )
#*************************
# Find required libraries
#*************************
ADD_DEFINITIONS(-DAPR_DECLARE_EXPORT)

SET(APR_FIND_REQUIRED ON)
SET(APR_FIND_QUIETLY ON)
SET(APRUTIL_FIND_REQUIRED OFF)
SET(APRUTIL_FIND_QUIETLY ON)
INCLUDE(FindAPR)


# TODO ---

#SET(LUA52_FIND_REQUIRED OFF)
#SET(LUA52_FIND_QUIETLY ON)
#INCLUDE(FindLua52)

IF (NOT LUA_FOUND )
    SET(LUA51_FIND_REQUIRED OFF)
    SET(LUA51_FIND_QUIETLY ON)
    INCLUDE(FindLua51)
ENDIF (NOT LUA_FOUND )

IF (NOT LUA_FOUND )
    SET(LUA50_FIND_REQUIRED ON)
    SET(LUA50_FIND_QUIETLY ON)
    INCLUDE(FindLua50)
ENDIF (NOT LUA_FOUND )

IF (NOT LUA_FOUND )
    MESSAGE(FATAL_ERROR "LUA is a required dependency to build NProxy")
ENDIF (NOT LUA_FOUND )

#*************************
# Last bits
#*************************

# Set the RPATH properly
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/nproxy/")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Don't add useless libraries to the modules
IF (NOT CMAKE_SYSTEM_NAME MATCHES "SunOS")
    SET(PROJECT_LINKER_OPTIONS "-Wl,--as-needed")
ENDIF (NOT CMAKE_SYSTEM_NAME MATCHES "SunOS")

#IF (APPLE)
#    SET(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib/nproxy/")
#ENDIF (APPLE)


#***************************************
# And the subdirs we have to recurse to
#***************************************

add_subdirectory (config)
add_subdirectory (lib)
add_subdirectory (src)
