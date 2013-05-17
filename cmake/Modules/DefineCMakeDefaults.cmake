# Put the include dirs which are in the source or build tree
# before all other include dirs, so the headers in the sources
# are prefered over the already installed ones
# since cmake 2.4.1
SET(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE ON)


# Use colored output
# since cmake 2.4.0

SET(CMAKE_COLOR_MAKEFILE ON)



# Define the generic version of the libraries here
SET(GENERIC_LIB_VERSION "0.1.0")
SET(GENERIC_LIB_SOVERSION "0")



# Set the default build type to release with debug info
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug
    CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
    FORCE
  )
ENDIF(NOT CMAKE_BUILD_TYPE)
