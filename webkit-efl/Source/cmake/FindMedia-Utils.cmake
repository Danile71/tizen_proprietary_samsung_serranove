# - Try to find Media-Utils
# Once done, this will define
#
#  Media-Utils_FOUND - system has libfms-util
#  Media-Utils_INCLUDE_DIRS - the libfms-util include directories
#  Media-Utils_LIBRARIES - link these to use libfms-util

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(Media-Utils_PKGCONF libmedia-utils)

# Include dir
find_path(Media-Utils_INCLUDE_DIR
  NAMES media-util-register.h
  PATHS ${Media-Utils_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES media-utils
)

# Finally the library itself
find_library(Media-Utils_LIBRARY
  NAMES media-utils
  PATHS ${Media-Utils_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(Media-Utils_PROCESS_INCLUDES Media-Utils_INCLUDE_DIR Media-Utils_INCLUDE_DIRS)
set(Media-Utils_PROCESS_LIBS Media-Utils_LIBRARY Media-Utils_LIBRARIES)
libfind_process(Media-Utils)
