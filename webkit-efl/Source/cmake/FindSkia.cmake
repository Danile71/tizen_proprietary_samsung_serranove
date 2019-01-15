# - Try to find Skia
# Once done, this will define
#
#  Skia_FOUND - system has Skia
#  Skia_INCLUDE_DIRS - the Skia include directories
#  Skia_LIBRARIES - link these to use Skia

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(Skia_PKGCONF skia)

# Include dir
SET(Skia_INCLUDE_DIR ${Skia_PKGCONF_INCLUDE_DIRS})

# Finally the library itself
find_library(Skia_LIBRARY
  NAMES skia
  PATHS ${Skia_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(Skia_PROCESS_INCLUDES Skia_INCLUDE_DIR)
set(Skia_PROCESS_LIBS Skia_LIBRARY)
libfind_process(Skia)

