# - Try to find SSL, CRYPTO
# Once done, this will define
#
#  OPENSSL_FOUND
#  OPENSSL_INCLUDE_DIRS
#  OPENSSL_LIBRARIES

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(OPENSSL_PKGCONF openssl)

# Include dir
find_path(OPENSSL_INCLUDE_DIR
  NAMES openssl/ssl.h
  PATHS ${OPENSSL_PKGCONF_INCLUDE_DIRS}
)

# Finally the library itself
find_library(SSL_LIBRARY
  NAMES ssl
  PATHS ${OPENSSL_PKGCONF_LIBRARY_DIRS}
)

find_library(CRYPTO_LIBRARY
  NAMES crypto
  PATHS ${OPENSSL_PKGCONF_LIBRARY_DIRS}
)

SET(OPENSSL_LIBRARY
  ${SSL_LIBRARY}
  ${CRYPTO_LIBRARY}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(OPENSSL_PROCESS_INCLUDES OPENSSL_INCLUDE_DIR OPENSSL_INCLUDE_DIRS)
set(OPENSSL_PROCESS_LIBS OPENSSL_LIBRARY OPENSSL_LIBRARIES)
libfind_process(OPENSSL)

