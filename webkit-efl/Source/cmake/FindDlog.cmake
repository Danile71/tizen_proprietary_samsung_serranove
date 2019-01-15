# - Try to find Dlog
# Once done, this will define
#
#  Dlog_FOUND - system has libdlog-0
#  Dlog_INCLUDE_DIRS - the libdlog-0 include directories
#  Dlog_LIBRARIES - link these to use libdlog-0

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(Dlog_PKGCONF dlog)

# Include dir
find_path(Dlog_INCLUDE_DIR
  NAMES dlog.h
  PATHS ${Dlog_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES dlog
)

# Finally the library itself
find_library(Dlog_LIBRARY
  NAMES dlog
  PATHS ${Dlog_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(Dlog_PROCESS_INCLUDES Dlog_INCLUDE_DIR Dlog_INCLUDE_DIRS)
set(Dlog_PROCESS_LIBS Dlog_LIBRARY Dlog_LIBRARIES)
libfind_process(Dlog)

