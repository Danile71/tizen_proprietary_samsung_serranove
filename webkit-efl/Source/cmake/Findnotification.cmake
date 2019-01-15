# - Try to notification
# Once done, this will define
#
#  notification_FOUND - system has notification
#  notification_INCLUDE_DIRS - the notification include directories
#  notification_LIBRARIES - link these to use notification

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(notification_PKGCONF notification)

# Include dir
find_path(notification_INCLUDE_DIR
  NAMES notification.h
  PATHS ${notification_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES notification
)

# Finally the library itself
find_library(notification_LIBRARY
  NAMES notification
  PATHS ${notification_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(notification_PROCESS_INCLUDES notification_INCLUDE_DIR notification_INCLUDE_DIRS)
set(notification_PROCESS_LIBS notification_LIBRARY notification_LIBRARIES)
libfind_process(notification)