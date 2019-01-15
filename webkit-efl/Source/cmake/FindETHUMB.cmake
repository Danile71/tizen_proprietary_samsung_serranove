# - Try to find ETHUMB
# Once done, this will define
#
#  ETHUMB_FOUND - system has libethumb
#  ETHUMB_INCLUDE_DIRS - the libethumb include directories
#  ETHUMB_LIBRARIES - link these to use libethumb

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(ETHUMB_PKGCONF ethumb)

# Include dir
find_path(ETHUMB_INCLUDE_DIR
  NAMES Ethumb.h
  PATHS ${ETHUMB_PKGCONF_INCLUDE_DIRS}
)

# Finally the library itself
find_library(ETHUMB_LIBRARY
  NAMES ethumb
  PATHS ${ETHUMB_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(ETHUMB_PROCESS_INCLUDES ETHUMB_INCLUDE_DIR ETHUMB_INCLUDE_DIRS)
set(ETHUMB_PROCESS_LIBS ETHUMB_LIBRARY ETHUMB_LIBRARIES)
libfind_process(ETHUMB)

