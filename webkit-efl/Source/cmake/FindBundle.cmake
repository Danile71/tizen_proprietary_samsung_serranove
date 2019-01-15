# - Try to find BUNDLE
# Once done, this will define
#
#  BUNDLE_FOUND - system has libbundle-0
#  BUNDLE_INCLUDE_DIRS - the libbundle-0 include directories
#  BUNDLE_LIBRARIES - link these to use libbundle-0

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(BUNDLE_PKGCONF bundle)

# Include dir
find_path(BUNDLE_INCLUDE_DIR
  NAMES bundle.h
  PATHS ${BUNDLE_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES bundle
)

# Finally the library itself
find_library(BUNDLE_LIBRARY
  NAMES bundle
  PATHS ${BUNDLE_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(BUNDLE_PROCESS_INCLUDES BUNDLE_INCLUDE_DIR BUNDLE_INCLUDE_DIRS)
set(BUNDLE_PROCESS_LIBS BUNDLE_LIBRARY BUNDLE_LIBRARIES)
libfind_process(BUNDLE)
