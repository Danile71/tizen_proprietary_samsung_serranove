# - Find ruby
# This module looks for ruby. This module defines the
# following values:
#  RUBY_EXECUTABLE: the full path to the ruby tool.
#  RUBY_FOUND: True if ruby has been found.

FIND_PROGRAM(RUBY_EXECUTABLE
    ruby
    DOC "path to the ruby executable"
)

# handle the QUIETLY and REQUIRED arguments and set RUBY_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Ruby DEFAULT_MSG RUBY_EXECUTABLE)

MARK_AS_ADVANCED(RUBY_EXECUTABLE)
