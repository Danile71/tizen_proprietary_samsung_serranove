SET(DumpRenderTree_SOURCES
    ${TOOLS_DIR}/DumpRenderTree/CyclicRedundancyCheck.cpp
    ${TOOLS_DIR}/DumpRenderTree/GCController.cpp
    ${TOOLS_DIR}/DumpRenderTree/LayoutTestController.cpp
    ${TOOLS_DIR}/DumpRenderTree/PixelDumpSupport.cpp
    ${TOOLS_DIR}/DumpRenderTree/WorkQueue.cpp
    ${TOOLS_DIR}/DumpRenderTree/cairo/PixelDumpSupportCairo.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/DumpHistoryItem.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/DumpRenderTree.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/DumpRenderTreeChrome.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/DumpRenderTreeView.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/EditingCallbacks.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/EventSender.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/FontManagement.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/GCControllerEfl.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/JSStringUtils.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/LayoutTestControllerEfl.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/PixelDumpSupportEfl.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/TextInputController.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/WorkQueueItemEfl.cpp
)

#if ENABLE(TIZEN_WEBKIT_EFL_DRT)
SET(ImageDiff_SOURCES
    ${TOOLS_DIR}/DumpRenderTree/efl/ImageDiff.cpp
)
#endif

#if ENABLE(TIZEN_WEBKIT_EFL_DRT)
SET(X11_LIBRARIES "X11")
SET(PANGO_LIBRARIES "pangoft2-1.0")
SET(CRYPTO_LIBRARIES "crypto")
#endif

SET(DumpRenderTree_LIBRARIES
    ${WTF_LIBRARY_NAME}
    ${JavaScriptCore_LIBRARY_NAME}
    ${WebCoreTestSupport_LIBRARY_NAME}
    ${WebCore_LIBRARY_NAME}
    ${WebKit_LIBRARY_NAME}
    ${WTF_LIBRARY_NAME}
    ${CAIRO_LIBRARIES}
    ${ECORE_LIBRARIES}
    ${ECORE_EVAS_LIBRARIES}
    ${ECORE_FILE_LIBRARIES}
    ${ECORE_INPUT_LIBRARIES}
    ${EDJE_LIBRARIES}
    ${EINA_LIBRARIES}
    ${EVAS_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    ${SQLITE_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${LIBSOUP_LIBRARIES}
#if ENABLE(TIZEN_WEBKIT_EFL_DRT)
    ${X11_LIBRARIES}
    ${PANGO_LIBRARIES}
    ${CRYPTO_LIBRARIES}
#endif
# To build WebKit1 needs below libraries.
    ${GLES20_LIBRARY}
    "-ldl"
)

#if ENABLE(TIZEN_WEBKIT_EFL_DRT)
SET(ImageDiff_LIBRARIES
    ${WTF_LIBRARY_NAME}
    ${JavaScriptCore_LIBRARY_NAME}
    ${WebCore_LIBRARY_NAME}
    ${WebKit_LIBRARY_NAME}
)
#endif

SET(DumpRenderTree_INCLUDE_DIRECTORIES
    ${WEBKIT_DIR}/efl/ewk
    ${WEBKIT_DIR}/efl
    ${WEBKIT_DIR}/efl/WebCoreSupport
    ${WEBCORE_DIR}
    ${WEBCORE_DIR}/accessibility
    ${WEBCORE_DIR}/bridge
    ${WEBCORE_DIR}/bridge/jsc
    ${WEBCORE_DIR}/bindings
    ${WEBCORE_DIR}/css
    ${WEBCORE_DIR}/dom
    ${WEBCORE_DIR}/editing
    ${WEBCORE_DIR}/html
    ${WEBCORE_DIR}/inspector
    ${WEBCORE_DIR}/notifications
    ${WEBCORE_DIR}/page
    ${WEBCORE_DIR}/page/animation
    ${WEBCORE_DIR}/platform
    ${WEBCORE_DIR}/platform/animation
    ${WEBCORE_DIR}/platform/text
    ${WEBCORE_DIR}/platform/graphics
    ${WEBCORE_DIR}/platform/graphics/cairo
    ${WEBCORE_DIR}/platform/graphics/transforms
    ${WEBCORE_DIR}/platform/graphics/filters
    ${WEBCORE_DIR}/platform/network
    ${WEBCORE_DIR}/platform/network/soup
    ${WEBCORE_DIR}/plugins
    ${WEBCORE_DIR}/plugins/tizen
    ${WEBCORE_DIR}/rendering
    ${WEBCORE_DIR}/rendering/style
    ${WEBCORE_DIR}/history
    ${WEBCORE_DIR}/loader
    ${WEBCORE_DIR}/loader/cache
    ${WEBCORE_DIR}/loader/icon
    ${WEBCORE_DIR}/svg
    ${WEBCORE_DIR}/svg/graphics
    ${WEBCORE_DIR}/svg/properties
    ${JAVASCRIPTCORE_DIR}
    ${JAVASCRIPTCORE_DIR}/API
    ${JAVASCRIPTCORE_DIR}/assembler
    ${JAVASCRIPTCORE_DIR}/bytecode
    ${JAVASCRIPTCORE_DIR}/dfg
    ${JAVASCRIPTCORE_DIR}/disassembler
    ${JAVASCRIPTCORE_DIR}/heap
    ${JAVASCRIPTCORE_DIR}/interpreter
    ${JAVASCRIPTCORE_DIR}/jit
    ${JAVASCRIPTCORE_DIR}/llint
    ${JAVASCRIPTCORE_DIR}/runtime
    ${JAVASCRIPTCORE_DIR}/ForwardingHeaders
    ${TOOLS_DIR}/DumpRenderTree
    ${TOOLS_DIR}/DumpRenderTree/cairo
    ${TOOLS_DIR}/DumpRenderTree/efl
    ${WTF_DIR}
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_BINARY_DIR}
    ${DERIVED_SOURCES_WEBCORE_DIR}
    ${WEBCORE_DIR}/bindings/js
    ${WEBCORE_DIR}/testing/js
    ${CAIRO_INCLUDE_DIRS}
    ${ECORE_INCLUDE_DIRS}
    ${ECORE_INCLUDE_DIRS}
    ${ECORE_EVAS_INCLUDE_DIRS}
    ${ECORE_FILE_INCLUDE_DIRS}
    ${ECORE_INPUT_INCLUDE_DIRS}
    ${EDJE_INCLUDE_DIRS}
    ${EINA_INCLUDE_DIRS}
    ${EVAS_INCLUDE_DIRS}
    ${FONTCONFIG_INCLUDE_DIR}
    ${GLIB_INCLUDE_DIRS}
    ${LIBSOUP_INCLUDE_DIRS}
)

# FIXME: DOWNLOADED_FONTS_DIR should not hardcode the directory
# structure. See <https://bugs.webkit.org/show_bug.cgi?id=81475>.
ADD_DEFINITIONS(-DFONTS_CONF_DIR="${TOOLS_DIR}/DumpRenderTree/gtk/fonts"
                -DDOWNLOADED_FONTS_DIR="${CMAKE_SOURCE_DIR}/WebKitBuild/Dependencies/Source/webkitgtk-test-fonts-0.0.3"
                -DDATA_DIR="${THEME_BINARY_DIR}")

IF (ENABLE_SLP_SUPPORT)
IF (ENABLE_NOTIFICATIONS)
    LIST(APPEND DumpRenderTree_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/notifications"
    )
ENDIF ()
ENDIF ()

# Override data directory. We always want to get a fresh theme.
REMOVE_DEFINITIONS(-DDATA_DIR=\"${DATA_DIR}\")
ADD_DEFINITIONS(-DDATA_DIR=\"${BUILD_DATA_DIR}\")

ADD_DEFINITIONS(-DFONTS_CONF_DIR="${TOOLS_DIR}/DumpRenderTree/gtk/fonts")

INCLUDE_DIRECTORIES(${DumpRenderTree_INCLUDE_DIRECTORIES})

ADD_EXECUTABLE(DumpRenderTree ${DumpRenderTree_SOURCES})
TARGET_LINK_LIBRARIES(DumpRenderTree ${DumpRenderTree_LIBRARIES})
SET_TARGET_PROPERTIES(DumpRenderTree PROPERTIES FOLDER "Tools")

ADD_EXECUTABLE(ImageDiff ${ImageDiff_SOURCES})
TARGET_LINK_LIBRARIES(ImageDiff ${DumpRenderTree_LIBRARIES})
SET_TARGET_PROPERTIES(ImageDiff PROPERTIES FOLDER "Tools")
