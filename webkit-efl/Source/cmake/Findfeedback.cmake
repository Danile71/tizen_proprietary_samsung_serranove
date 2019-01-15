# - Try to find feedback
# Once done, this will define
#
#  feedback_FOUND - system has feedback
#  feedback_INCLUDE_DIRS - the feedback include directories

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(feedback_PKGCONF feedback)

# Include dir
find_path(feedback_INCLUDE_DIR
  NAMES feedback.h
  PATHS ${feedback_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES feedback
)

# Set the include dir variables.
set(feedback_PROCESS_INCLUDES feedback_INCLUDE_DIR feddback_INCLUDE_DIRS)
