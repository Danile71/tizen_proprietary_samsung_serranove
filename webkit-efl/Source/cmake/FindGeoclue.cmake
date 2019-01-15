# - Try to find Geoclue
# Once done, this will define
#
#  Geoclue_FOUND - system has libgeoclue
#  Geoclue_INCLUDE_DIRS - the libgeoclue include directories
#  Geoclue_LIBRARIES - link these to use libfms-util

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(Geoclue_PKGCONF geoclue)

# Include dir
find_path(Geoclue_INCLUDE_DIR
  NAMES geoclue-master.h
  PATHS ${Geoclue_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES geoclue
)

# Finally the library itself
find_library(Geoclue_LIBRARY
  NAMES geoclue
  PATHS ${Geoclue_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(Geoclue_PROCESS_INCLUDES Geoclue_INCLUDE_DIR Geoclue_INCLUDE_DIRS)
set(Geoclue_PROCESS_LIBS Geoclue_LIBRARY Geoclue_LIBRARIES)
libfind_process(Geoclue)

