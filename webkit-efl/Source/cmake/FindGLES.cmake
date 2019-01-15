# - Try to find GLES20
# Once done, this will define
#
#  GLES20_FOUND - system has GLES20
#  GLES20_INCLUDE_DIRS - the GLES20 include directories
#  GLES20_LIBRARIES - link these to use GLES20

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(GLES20_PKGCONF gles20)

# Include dir
find_path(GLES20_INCLUDE_DIR
  NAMES GLES2/gl2.h
  PATHS ${GLES20_PKGCONF_INCLUDE_DIRS}
)

# Finally the library itself
find_library(GLES_LIBRARY
  NAMES GLESv2
  PATHS ${GLES20_PKGCONF_LIBRARY_DIRS}
)

find_library(EGL_LIBRARY
  NAMES EGL
  PATHS ${GLES20_PKGCONF_LIBRARY_DIRS}
)

# Include dir
find_path(EGL_INCLUDE_DIR
  NAMES EGL/eglext.h
  PATHS ${GLES20_PKGCONF_INCLUDE_DIRS}
)

SET(GLES20_LIBRARY 
    ${GLES_LIBRARY}
    ${EGL_LIBRARY}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(GLES20_PROCESS_INCLUDES GLES20_INCLUDE_DIR GLES20_INCLUDE_DIRS)
set(GLES20_PROCESS_LIBS GLES20_LIBRARY GLES20_LIBRARIES)
libfind_process(GLES20)

