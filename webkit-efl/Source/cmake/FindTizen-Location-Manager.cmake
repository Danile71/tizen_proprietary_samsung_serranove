# - Try to find Tizen-Location-Manager
# Once done, this will define
#
#  Tizen-Location-Manager_FOUND - system has location-manager
#  Tizen-Location-Manager_INCLUDE_DIRS - the location-manager include directories
#  Tizen-Location-Manager_LIBRARIES - link these to use location-manager

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(Tizen-Location-Manager_PKGCONF capi-location-manager)

# Include dir
find_path(Tizen-Location-Manager_INCLUDE_DIR
  NAMES locations.h
  PATHS ${Tizen-Location-Manager_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES location
)

# Finally the library itself
find_library(Tizen-Location-Manager_LIBRARY
  NAMES capi-location-manager
  PATHS ${Tizen-Location-Manager_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(Tizen-Location-Manager_PROCESS_INCLUDES Tizen-Location-Manager_INCLUDE_DIR Tizen-Location-Manager_INCLUDE_DIRS)
set(Tizen-Location-Manager_PROCESS_LIBS Tizen-Location-Manager_LIBRARY Tizen-Location-Manager_LIBRARIES)
libfind_process(Tizen-Location-Manager)
