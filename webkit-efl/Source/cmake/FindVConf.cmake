# - Try to find VConf
# Once done, this will define
#
#  VConf_FOUND - system has libvconf-0
#  VConf_INCLUDE_DIRS - the libvconf-0 include directories
#  VConf_LIBRARIES - link these to use libvconf-0

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(VConf_PKGCONF vconf)

# Include dir
find_path(VConf_INCLUDE_DIR
  NAMES vconf.h
  PATHS ${VConf_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES vconf
)

# Finally the library itself
find_library(VConf_LIBRARY
  NAMES vconf
  PATHS ${VConf_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(VConf_PROCESS_INCLUDES VConf_INCLUDE_DIR VConf_INCLUDE_DIRS)
set(VConf_PROCESS_LIBS VConf_LIBRARY VConf_LIBRARIES)
libfind_process(VConf)

