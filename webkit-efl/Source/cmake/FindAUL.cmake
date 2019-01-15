# - Try to find AUL
# Once done, this will define
#
#  AUL_FOUND - system has libaul-1
#  AUL_INCLUDE_DIRS - the libaul-1 include directories
#  AUL_LIBRARIES - link these to use libaul-1

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(AUL_PKGCONF aul)

# Include dir
find_path(AUL_INCLUDE_DIR
  NAMES aul.h
  PATHS ${AUL_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES aul
)

# Finally the library itself
find_library(AUL_LIBRARY
  NAMES aul
  PATHS ${AUL_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(AUL_PROCESS_INCLUDES AUL_INCLUDE_DIR AUL_INCLUDE_DIRS)
set(AUL_PROCESS_LIBS AUL_LIBRARY AUL_LIBRARIES)
libfind_process(AUL)

