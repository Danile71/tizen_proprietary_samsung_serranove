# - Try to find STT

include(LibFindMacros)

libfind_pkg_check_modules(STT_PKGCONF stt)

find_path(STT_INCLUDE_DIR
    NAMES stt.h
    PATHS ${STT_PKGCONF_INCLUDE_DIRS}
)

find_library(STT_LIBRARY
    NAMES stt
    PATHS ${STT_PKGCONF_LIBRARY_DIRS}
)

set(STT_PROCESS_INCLUDES STT_INCLUDE_DIR STT_INCLUDE_DIRS)
set(STT_PROCESS_LIBS STT_LIBRARY STT_LIBRARIES)
libfind_process(STT)
