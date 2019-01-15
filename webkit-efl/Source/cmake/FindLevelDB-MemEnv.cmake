# - Try to find LevelDB
# Once done, this will define
#
#  LevelDB-MemEnv_FOUND - system has LevelDB-MemEnv
#  LevelDB-MemEnv_INCLUDE_DIRS - the LevelDB-MemEnv include directories
#  LevleDB-MemEnv_LIBRARIES - link these to use LevelDB-MemEnv

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(LevelDB-MemEnv_PKGCONF leveldb)

# Include dir
find_path(LevelDB-MemEnv_INCLUDE_DIR
  NAMES helpers/memenv/memenv.h
  PATHS ${LevelDB-MemEnv_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES helpers/memenv
)

# Finally the library itself
find_library(LevelDB-MemEnv_LIBRARY
  NAMES memenv
  PATHS ${LevelDB-MemEnv_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(LevelDB-MemEnv_PROCESS_INCLUDES LevelDB-MemEnv_INCLUDE_DIR)
set(LevelDB-MemEnv_PROCESS_LIBS LevelDB-MemEnv_LIBRARY)
libfind_process(LevelDB-MemEnv)
