SET(fw_test "openal_test")

SET(dependents "dlog glib-2.0 freealut")
SET(pc_dependents "dlog glib-2.0 freealut")

INCLUDE(FindPkgConfig)
FOREACH(flag ${${fw_test}_CFLAGS})
    SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -Wall -Werror")
MESSAGE("CMAKE_C_FLAGS ${CMAKE_C_FLAGS}")

ADD_EXECUTABLE(openal_test open_al_test.c)
TARGET_LINK_LIBRARIES(openal_test openal alut ${${fw_test}_LDFLAGS})
INSTALL(TARGETS openal_test  DESTINATION /usr/bin)
