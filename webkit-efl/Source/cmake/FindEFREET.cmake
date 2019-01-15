# - Try to find EFREET
# Once done, this will define
#
#  EFREET_FOUND - system has efreet
#  EFREET_INCLUDE_DIRS - the efreet include directories
#  EFREET_LIBRARIES - link these to use efreet

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(EFREET_PKGCONF efreet)

# Include dir
find_path(EFREET_INCLUDE_DIR
  NAMES Efreet.h
  PATHS ${EFREET_PKGCONF_INCLUDE_DIRS}
)

# Finally the library itself
find_library(EFREET_LIBRARY
  NAMES efreet
  PATHS ${EFREET_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(EFREET_PROCESS_INCLUDES EFREET_INCLUDE_DIR EFREET_INCLUDE_DIRS)
set(EFREET_PROCESS_LIBS EFREET_LIBRARY EFREET_LIBRARIES)
libfind_process(EFREET)

