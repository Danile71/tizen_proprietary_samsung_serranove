# - Try to find Pixman-1
# Once done, this will define
#
#  PIXMAN_FOUND - system has pixman
#  PIXMAN_INCLUDE_DIRS - the pixman include directories
#  PIXMAN_LIBRARIES - link these to use pixman

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(PIXMAN_PKGCONF pixman-1)

# Include dir
find_path(PIXMAN_INCLUDE_DIR
  NAMES pixman.h
  PATHS ${PIXMAN_PKGCONF_INCLUDE_DIRS}
)

# Finally the library itself
find_library(PIXMAN_LIBRARY
  NAMES pixman-1
  PATHS ${PIXMAN_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(PIXMAN_PROCESS_INCLUDES PIXMAN_INCLUDE_DIR)
set(PIXMAN_PROCESS_LIBS PIXMAN_LIBRARY)

libfind_process(PIXMAN)
