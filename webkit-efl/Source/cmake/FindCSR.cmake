# - Try to find CSR
# Once done, this will define
#
#  CSR_FOUND - system has libsecfw
#  CSR_INCLUDE_DIRS - the libsecfw include directories
#  CSR_LIBRARIES - link these to use libsecfw

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(CSR_PKGCONF csr-framework)

# Include dir
find_path(CSR_INCLUDE_DIR
  NAMES TCSImpl.h
  PATHS ${CSR_PKGCONF_INCLUDE_DIRS}
)

# Finally the library itself
find_library(CSR_LIBRARY
  NAMES secfw
  PATHS ${CSR_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(CSR_PROCESS_INCLUDES CSR_INCLUDE_DIR CSR_INCLUDE_DIRS)
set(CSR_PROCESS_LIBS CSR_LIBRARY CSR_LIBRARIES)
libfind_process(CSR)

