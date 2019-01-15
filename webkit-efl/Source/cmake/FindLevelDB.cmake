# - Try to find LevelDB
# Once done, this will define
#
#  LevelDB_FOUND - system has LevelDB
#  LevelDB_INCLUDE_DIRS - the LevelDB include directories
#  LevleDB_LIBRARIES - link these to use LevelDB

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(LevelDB_PKGCONF leveldb)

# Include dir
find_path(LevelDB_INCLUDE_DIR
  NAMES leveldb/db.h
  PATHS ${LevelDB_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES leveldb
)

# Finally the library itself
find_library(LevelDB_LIBRARY
  NAMES leveldb
  PATHS ${LevelDB_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(LevelDB_PROCESS_INCLUDES LevelDB_INCLUDE_DIR)
set(LevelDB_PROCESS_LIBS LevelDB_LIBRARY)
libfind_process(LevelDB)
