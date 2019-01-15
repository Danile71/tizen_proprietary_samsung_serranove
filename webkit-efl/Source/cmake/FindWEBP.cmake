# - Try to find ewebkit2-ext
# Once done, this will define
#
#  WEBP_FOUND - system has WEBP_INFO
#  WEBP_INCLUDE_DIRS - the WEBP include directories
#  WEBP_LIBRARIES - link these to use WEBP_INFO

INCLUDE(FindPkgConfig)

PKG_CHECK_MODULES(WEBP
    libwebp
)

FIND_LIBRARY(WEBP_LIBRARIES NAMES libwebp
HINTS ${WEBP_LIBRARY_DIRS} ${WEBP_LIBDIR}
)

SET(WEBP_LIBRARIES
${WEBP_LIBRARIES}
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(WEBP DEFAULT_MSG WEBP_LIBRARIES)
