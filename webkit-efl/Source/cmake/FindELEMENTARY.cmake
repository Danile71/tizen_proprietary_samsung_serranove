# - Try to find ELEMENTARY
# Once done, this will define
#
#  ELEMENTARY_FOUND - system has libelm
#  ELEMENTARY_INCLUDE_DIRS - the libelm include directories
#  ELEMENTARY_LIBRARIES - link these to use libelm

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(ELEMENTARY_PKGCONF elementary)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(ELEMENTARY_PROCESS_INCLUDES ELEMENTARY_PKGCONF_INCLUDE_DIRS)
set(ELEMENTARY_PROCESS_LIBS ELEMENTARY_PKGCONF_LIBRARIES)
libfind_process(ELEMENTARY)

