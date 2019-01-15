# - Try to find TTS
# Once done, this will define
#
#  TTS_FOUND - system has libtts
#  TTS_INCLUDE_DIRS - the libtts include directories
#  TTS_LIBRARIES - link these to use libtts

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(TTS_PKGCONF tts)

# Include dir
find_path(TTS_INCLUDE_DIR
  NAMES tts.h
  PATHS ${TTS_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES tts
)

# Finally the library itself
find_library(TTS_LIBRARY
  NAMES tts
  PATHS ${TTS_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(TTS_PROCESS_INCLUDES TTS_INCLUDE_DIR TTS_INCLUDE_DIRS)
set(TTS_PROCESS_LIBS TTS_LIBRARY TTS_LIBRARIES)
libfind_process(TTS)

