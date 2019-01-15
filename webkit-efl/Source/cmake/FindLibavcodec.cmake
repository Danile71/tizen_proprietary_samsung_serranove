include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(Libavcodec_PKGCONF libavcodec)

set(Libavcodec_PKGCONF "/usr/lib/pkgconfig/")
set(Libavcodec_INCLUDE_DIR "/usr/include/libavcodec/")
set(Libavcodec_LIBRARY_DIR "/usr/lib/")

# Finally the library itself
find_library(Libavcodec_LIBRARY
  NAMES libavcodec
  PATHS ${Libavcodec_LIBRARY_DIR}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this lib depends on.
set(Libavcodec_INCLUDES Libavcodec_INCLUDE_DIR)
set(Libavcodec_LIBS "${Libavcodec_LIBRARY_DIR}libavcodec.so")

  MESSAGE("Libavcodec_LIBS-->" "${Libavcodec_LIBS}")
  MESSAGE("Libavcodec_INCLUDE_DIR-->" "${Libavcodec_INCLUDE_DIR}")
  MESSAGE("Libavcodec_PKGCONF-->" "${Libavcodec_PKGCONF}")

libfind_process(libavcodec)

