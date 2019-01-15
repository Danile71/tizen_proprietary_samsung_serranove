LIST(APPEND WebKit2StaticForDebug_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/Modules/filesystem"
    "${WEBCORE_DIR}/Modules/mediastream"
    "${WEBCORE_DIR}/page/scrolling"
    "${WEBCORE_DIR}/page/scrolling/coordinatedgraphics"
    "${WEBCORE_DIR}/platform/audio/gstreamer/tizen"
    "${WEBCORE_DIR}/platform/efl/tizen"
    "${WEBCORE_DIR}/platform/graphics/efl/tizen"
    "${WEBCORE_DIR}/platform/graphics/gstreamer"
    "${WEBCORE_DIR}/platform/graphics/gstreamer/tizen"
    "${WEBCORE_DIR}/platform/graphics/surfaces"
    "${WEBCORE_DIR}/platform/graphics/texmap/tizen"
    "${WEBCORE_DIR}/platform/mediastream"
    "${WEBKIT2_DIR}/Platform/tizen/AboutData"
    "${WEBKIT2_DIR}/Shared/efl/tizen"
    "${WEBKIT2_DIR}/Shared/API/c/cairo"
    "${WEBKIT2_DIR}/Shared/API/c/efl"
    "${WEBKIT2_DIR}/Shared/API/c/tizen"
    "${WEBKIT2_DIR}/Shared/tizen"
    "${WEBKIT2_DIR}/UIProcess/API/C/efl"
    "${WEBKIT2_DIR}/UIProcess/API/C/efl/tizen"
    "${WEBKIT2_DIR}/UIProcess/API/efl/editor"
    "${WEBKIT2_DIR}/UIProcess/API/efl/tizen"
    "${WEBKIT2_DIR}/UIProcess/efl"
    "${WEBKIT2_DIR}/UIProcess/MediaStream"
    "${WEBKIT2_DIR}/UIProcess/texmap"
    "${WEBKIT2_DIR}/UIProcess/tizen"
    "${WEBKIT2_DIR}/WebProcess/LocalFileSystem"
    "${WEBKIT2_DIR}/WebProcess/MediaStream"
    "${WEBKIT2_DIR}/WebProcess/WebPage/efl/tizen"
    "${WTF_DIR}"
    "${WTF_DIR}/wtf/efl/tizen"
    ${ASM_INCLUDE_DIRS}
    ${CAPI_INCLUDE_DIRS}
    ${ELEMENTARY_INCLUDE_DIRS}
    ${MM-Player_INCLUDE_DIRS}
    ${TTS_INCLUDE_DIRS}
    ${Tizen-Location-Manager_INCLUDE_DIRS}
    ${VConf_INCLUDE_DIRS}
    ${WKEXT_INCLUDE_DIRS}
    ${TBM_INCLUDE_DIRS}
    ${DRI2_INCLUDE_DIRS}
    ${UTILX_INCLUDE_DIRS}
    ${EFL_ASSIST_INCLUDE_DIRS}
    ${LIBSMACK_INCLUDE_DIRS}
    ${DRM_INCLUDE_DIRS}
    ${PIXMAN_INCLUDE_DIRS}
    ${notification_INCLUDE_DIRS}
    ${ATK_INCLUDE_DIRS}
    ${ATK_BRIDGE_INCLUDE_DIRS}
)

LIST(APPEND WebKit2_LIBRARIES
    ${CAPI_LIBRARIES}
    ${ELEMENTARY_LIBRARIES}
    ${MM-Player_LIBRARIES} 
    ${TTS_LIBRARIES}
    ${Tizen-Location-Manager_LIBRARIES}
    ${VConf_LIBRARIES} 
    ${WKEXT_LIBRARIES}
    ${UTILX_LIBRARIES}
    ${LIBSMACK_LIBRARIES}
    ${notification_LIBRARIES}
    ${ATK_LIBRARIES}
    ${ATK_BRIDGE_LIBRARIES}
)

ADD_DEFINITIONS(-DWTF_USE_CROSS_PLATFORM_CONTEXT_MENUS=0)
ADD_DEFINITIONS(-DENABLE_CONTEXT_MENUS=1)

LIST(REMOVE_ITEM WebKit2StaticForDebug_SOURCES
    Shared/efl/NativeWebKeyboardEventEfl.cpp
    WebProcess/WebCoreSupport/efl/WebErrorsEfl.cpp
)

LIST(APPEND WebKit2StaticForDebug_SOURCES
    Platform/tizen/AboutData/AboutDataTizen.cpp

    PluginProcess/efl/PluginControllerProxyEfl.cpp

    Shared/API/c/tizen/WKURLRequestTizen.cpp
    Shared/API/c/tizen/WKURLResponseTizen.cpp
    Shared/efl/tizen/WebSubresourceTizen.cpp
    Shared/Plugins/Netscape/x11/NetscapePluginModuleX11.cpp
    Shared/tizen/ArgumentCodersTizen.cpp
    Shared/tizen/NativeWebKeyboardEventTizen.cpp
    Shared/tizen/ProcessSmackLabel.cpp
    Shared/tizen/WebFormData.cpp
    Shared/tizen/WebURLRequestTizen.cpp
    Shared/tizen/WebURLResponseTizen.cpp

    UIProcess/API/C/efl/tizen/WKLocalFileSystemManager.cpp
    UIProcess/API/C/efl/tizen/WKContextTizen.cpp
    UIProcess/API/C/efl/tizen/WKIconDatabaseTizen.cpp
    UIProcess/API/C/efl/tizen/WKPageTizen.cpp
    UIProcess/API/C/efl/tizen/WKPreferencesTizen.cpp
    UIProcess/API/C/WKUserMediaPermissionRequest.cpp

    UIProcess/API/efl/ewk_auth_challenge.cpp
    UIProcess/API/efl/ewk_certificate.cpp
    UIProcess/API/efl/ewk_content_screening_detection.cpp
    UIProcess/API/efl/ewk_console_message.cpp
    UIProcess/API/efl/ewk_context_injected_bundle_client.cpp
    UIProcess/API/efl/ewk_context_menu.cpp
    UIProcess/API/efl/ewk_custom_handlers.cpp
    UIProcess/API/efl/ewk_form_data.cpp
    UIProcess/API/efl/ewk_frame.cpp
    UIProcess/API/efl/ewk_geolocation.cpp
    UIProcess/API/efl/ewk_geolocation_provider.cpp
    UIProcess/API/efl/ewk_history.cpp
    UIProcess/API/efl/ewk_hit_test.cpp
    UIProcess/API/efl/ewk_intercept_request.cpp
    UIProcess/API/efl/ewk_notification.cpp
    UIProcess/API/efl/ewk_notification_provider.cpp
    UIProcess/API/efl/ewk_policy_decision.cpp
    UIProcess/API/efl/ewk_security_origin.cpp
    UIProcess/API/efl/ewk_text_style.cpp
    UIProcess/API/efl/ewk_user_media.cpp
    UIProcess/API/efl/ewk_util.cpp
    UIProcess/API/efl/ewk_view_context_menu_client.cpp
    UIProcess/API/efl/ewk_view_icondatabase_client.cpp
    UIProcess/API/efl/ewk_view_tizen_client.cpp
    UIProcess/API/efl/ewk_view_utilx.cpp
    UIProcess/API/efl/ewk_web_application_icon_data.cpp
    UIProcess/API/efl/ewk_autofill_profile.cpp

    UIProcess/API/efl/tizen/ApplicationCachePermissionPopup.cpp
    UIProcess/API/efl/tizen/ClipboardHelper.cpp
    UIProcess/API/efl/tizen/Drag.cpp
    UIProcess/API/efl/tizen/DragHandle.cpp
    UIProcess/API/efl/tizen/EasingUtilities.cpp
    UIProcess/API/efl/tizen/EdgeEffect.cpp
    UIProcess/API/efl/tizen/ExceededDatabaseQuotaPermissionPopup.cpp
    UIProcess/API/efl/tizen/ExceededIndexedDatabaseQuotaPermissionPopup.cpp
    UIProcess/API/efl/tizen/ExceededLocalFileSystemQuotaPermissionPopup.cpp
    UIProcess/API/efl/tizen/Flick.cpp
    UIProcess/API/efl/tizen/FocusRing.cpp
    UIProcess/API/efl/tizen/AutoFillPopup.cpp
    UIProcess/API/efl/tizen/PasswordSaveConfirmPopup.cpp
    UIProcess/API/efl/tizen/GeolocationPermissionPopup.cpp
    UIProcess/API/efl/tizen/GestureClient.cpp
    UIProcess/API/efl/tizen/GestureRecognizer.cpp
    UIProcess/API/efl/tizen/InputFieldZoom.cpp
    UIProcess/API/efl/tizen/InputPicker.cpp
    UIProcess/API/efl/tizen/JavaScriptPopup.cpp
    UIProcess/API/efl/tizen/LinkMagnifierProxy.cpp
    UIProcess/API/efl/tizen/NotificationManagerEfl.cpp
    UIProcess/API/efl/tizen/NotificationPermissionPopup.cpp
    UIProcess/API/efl/tizen/OfflinePageSave.cpp
    UIProcess/API/efl/tizen/OpenPanel.cpp
    UIProcess/API/efl/tizen/OpenMediaPlayer.cpp
    UIProcess/API/efl/tizen/Pan.cpp
    UIProcess/API/efl/tizen/PermissionPopupManager.cpp
    UIProcess/API/efl/tizen/FormDatabase.cpp
    UIProcess/API/efl/tizen/ScreenReaderProxy.cpp
    UIProcess/API/efl/tizen/SmartZoom.cpp
    UIProcess/API/efl/tizen/TextSelection.cpp
    UIProcess/API/efl/tizen/TextSelectionHandle.cpp
    UIProcess/API/efl/tizen/TextSelectionMagnifier.cpp
    UIProcess/API/efl/tizen/UserMediaPermissionPopup.cpp
    UIProcess/API/efl/tizen/Zoom.cpp
    UIProcess/API/efl/tizen/ewk_popup_picker.cpp
    UIProcess/API/efl/tizen/ProfileFormAutoFill.cpp

    UIProcess/efl/PlatformSurfaceTexturePoolEfl.cpp

    UIProcess/MediaStream/UserMediaPermissionRequest.cpp
    UIProcess/MediaStream/UserMediaPermissionRequestManagerProxy.cpp

    UIProcess/texmap/tizen/LayerBackingStoreTizen.cpp

    UIProcess/tizen/MainFrameScrollbarTizen.cpp
    UIProcess/tizen/WebContextMenuProxyTizen.cpp

    UIProcess/tizen/WebLayerTreeRendererTizen.cpp
    UIProcess/tizen/WebLocalFileSystemManagerProxy.cpp
    UIProcess/tizen/WebTizenClient.cpp

    WebProcess/FullScreen/efl/WebFullScreenManagerEfl.cpp

    WebProcess/LocalFileSystem/WebLocalFileSystemManager.cpp

    WebProcess/MediaStream/UserMediaPermissionRequestManager.cpp

    WebProcess/Plugins/Netscape/x11/NetscapePluginX11.cpp
    WebProcess/Plugins/Netscape/efl/PluginProxyEfl.cpp

    WebProcess/WebCoreSupport/WebRegisterContentHandlerClient.cpp
    WebProcess/WebCoreSupport/WebRegisterProtocolHandlerClient.cpp
    WebProcess/WebCoreSupport/WebUserMediaClient.cpp
    WebProcess/WebCoreSupport/tizen/WebDragClientTizen.cpp
    WebProcess/WebCoreSupport/tizen/WebErrorsTizen.cpp

    WebProcess/WebPage/cairo/RecordingSurfaceSetCairo.cpp
    WebProcess/WebPage/efl/tizen/LinkMagnifier.cpp
    WebProcess/WebPage/efl/tizen/PlatformSurfacePoolTizen.cpp
    WebProcess/WebPage/efl/tizen/ScreenReader.cpp
    WebProcess/WebPage/efl/tizen/TiledBackingStoreRemoteTileTizen.cpp
    WebProcess/WebPage/efl/tizen/WebBaseAccessibilityTizen.cpp
    WebProcess/WebPage/efl/tizen/WebPageAccessibilityObjectTizen.cpp
    WebProcess/WebPage/efl/tizen/WebPageSerializerTizen.cpp
)

IF (WTF_USE_TEXTURE_MAPPER)
    LIST(APPEND WebKit2StaticForDebug_SOURCES
        UIProcess/texmap/LayerBackingStore.cpp
    )
ENDIF ()

LIST(APPEND WebKit2StaticForDebug_MESSAGES_IN_FILES
    UIProcess/WebLocalFileSystemManagerProxy.messages.in
    WebProcess/LocalFileSystem/WebLocalFileSystemManager.messages.in
)

SET(THEME_DIR ${CMAKE_BINARY_DIR}/theme)
SET(CONTROL_THEME ${THEME_DIR}/control.edj)
SET(JS_POPUP_THEME ${THEME_DIR}/JavaScriptPopup.edj)

IF (ENABLE_TIZEN_INDEXED_DATABASE)
LIST(APPEND WebKit2StaticForDebug_INCLUDE_DIRECTORIES
    "${WEBKIT2_DIR}/WebProcess/IndexedDatabase"
)

LIST(APPEND WebKit2StaticForDebug_SOURCES
    UIProcess/API/C/efl/tizen/WKIndexedDatabaseManager.cpp
    UIProcess/tizen/WebIndexedDatabaseManagerProxy.cpp
    WebProcess/IndexedDatabase/WebIndexedDatabaseManager.cpp
)

LIST(APPEND WebKit2StaticForDebug_MESSAGES_IN_FILES
    UIProcess/WebIndexedDatabaseManagerProxy.messages.in
    WebProcess/IndexedDatabase/WebIndexedDatabaseManager.messages.in
)
ENDIF ()

IF (ENABLE_TIZEN_OPEN_PANEL)
    LIST(APPEND WebKit2StaticForDebug_INCLUDE_DIRECTORIES
        ${UIGadget_INCLUDE_DIRS}
    )
    LIST(APPEND WebKit2_LIBRARIES
        ${UIGadget_LIBRARY}
    )
ENDIF ()

IF (ENABLE_TIZEN_WEBPROCESS_MEM_TRACK)
    LIST(APPEND WebKit2StaticForDebug_INCLUDE_DIRECTORIES
        ${AUL_INCLUDE_DIRS}
    )
    LIST(APPEND WebKit2_LIBRARIES
        ${AUL_LIBRARIES}
    )
ENDIF ()

IF (ENABLE_TIZEN_WEBKIT2_TEXT_SELECTION)
    SET(MAGNIFIER_THEME ${THEME_DIR}/Magnifier.edj)
ENDIF ()

IF (ENABLE_TIZEN_DRAG_SUPPORT)
    SET(DRAG_THEME ${THEME_DIR}/Drag.edj)
ENDIF ()

IF (ENABLE_TIZEN_WEBKIT2_FORM_DATABASE)
    SET(FORMDATA_THEME ${THEME_DIR}/FormDataPopup.edj)
ENDIF ()

IF (ENABLE_TIZEN_EDGE_SUPPORT)
    SET(EDGE_THEME ${THEME_DIR}/Edge.edj)
ENDIF ()

IF (ENABLE_TIZEN_LINK_MAGNIFIER)
    SET(LINK_MAGNIFIER_THEME ${THEME_DIR}/LinkMagnifier.edj)
ENDIF ()

FILE(MAKE_DIRECTORY ${THEME_DIR})

ADD_CUSTOM_COMMAND(
    OUTPUT ${CONTROL_THEME}
    COMMAND ${EDJE_CC_EXECUTABLE} -id ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/images ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/control.edc ${CONTROL_THEME}
    DEPENDS
        ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/control.edc
)

ADD_CUSTOM_COMMAND(
    OUTPUT ${JS_POPUP_THEME}
    COMMAND  ${EDJE_CC_EXECUTABLE} -id ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/images ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/JavaScriptPopup.edc ${JS_POPUP_THEME}
    DEPENDS
        ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/JavaScriptPopup.edc
)

IF (ENABLE_TIZEN_WEBKIT2_TEXT_SELECTION)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${MAGNIFIER_THEME}
        COMMAND  ${EDJE_CC_EXECUTABLE} -id ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/images ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/Magnifier.edc ${MAGNIFIER_THEME}
        DEPENDS
            ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/Magnifier.edc
    )
    LIST(APPEND WebKit2StaticForDebug_SOURCES ${MAGNIFIER_THEME})
ENDIF ()

IF (ENABLE_TIZEN_DRAG_SUPPORT)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${DRAG_THEME}
        COMMAND  ${EDJE_CC_EXECUTABLE} -id ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/images ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/Drag.edc ${DRAG_THEME}
        DEPENDS
            ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/Drag.edc
    )
    LIST(APPEND WebKit2StaticForDebug_SOURCES ${DRAG_THEME})
ENDIF ()

IF (ENABLE_TIZEN_WEBKIT2_FORM_DATABASE)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${FORMDATA_THEME}
        COMMAND  ${EDJE_CC_EXECUTABLE} -id ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/images ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/FormDataPopup.edc ${FORMDATA_THEME}
        DEPENDS
            ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/FormDataPopup.edc
    )
    LIST(APPEND WebKit2StaticForDebug_SOURCES ${FORMDATA_THEME})
ENDIF ()

IF (ENABLE_TIZEN_EDGE_SUPPORT)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${EDGE_THEME}
        COMMAND  ${EDJE_CC_EXECUTABLE} -id ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/images/overscrolling/dark ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/Edge.edc ${EDGE_THEME}
        DEPENDS
            ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/Edge.edc
    )
LIST(APPEND WebKit2StaticForDebug_SOURCES ${EDGE_THEME})
ENDIF ()

IF (ENABLE_TIZEN_LINK_MAGNIFIER)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${LINK_MAGNIFIER_THEME}
        COMMAND ${EDJE_CC_EXECUTABLE} -id ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/images ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/LinkMagnifier.edc ${LINK_MAGNIFIER_THEME}
        DEPENDS ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/LinkMagnifier.edc
    )
    LIST(APPEND WebKit2StaticForDebug_SOURCES ${LINK_MAGNIFIER_THEME})
ENDIF ()

LIST(APPEND WebKit2StaticForDebug_SOURCES ${CONTROL_THEME} ${JS_POPUP_THEME})

ADD_DEFINITIONS("-DEDJE_DIR=\"${CMAKE_INSTALL_PREFIX}/share/${WebKit2_LIBRARY_NAME}-${PROJECT_VERSION_MAJOR}/themes\"")

FILE(GLOB Shared_API_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/Shared/API/c/*.h")
LIST(APPEND Shared_API_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/Shared/API/c/efl/WKBaseEfl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shared/API/c/cairo/WKImageCairo.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shared/API/c/tizen/WKURLRequestTizen.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Shared/API/c/tizen/WKURLResponseTizen.h"
)

LIST(APPEND NPAPI_HEADERS
    "${WEBCORE_DIR}/plugins/npapi.h"
    "${WEBCORE_DIR}/plugins/npfunctions.h"
    "${WEBCORE_DIR}/plugins/npruntime.h"
    "${WEBCORE_DIR}/plugins/nptypes.h"
)

INCLUDE_IF_EXISTS(${WEBKIT2_DIR}/PlatformExperimental.cmake)

INSTALL(FILES ${NPAPI_HEADERS}
    DESTINATION include/${WebKit2_LIBRARY_NAME}-${PROJECT_VERSION_MAJOR}/NPAPI)

INSTALL(FILES ${Shared_API_HEADERS}
    DESTINATION include/${WebKit2_LIBRARY_NAME}-${PROJECT_VERSION_MAJOR}/WebKit2)

INSTALL(FILES ${CONTROL_THEME}
    DESTINATION share/${WebKit2_LIBRARY_NAME}-${PROJECT_VERSION_MAJOR}/themes)
INSTALL(FILES ${JS_POPUP_THEME}
    DESTINATION share/${WebKit2_LIBRARY_NAME}-${PROJECT_VERSION_MAJOR}/themes)

IF (ENABLE_TIZEN_WEBKIT2_TEXT_SELECTION)
    INSTALL(FILES ${MAGNIFIER_THEME}
        DESTINATION share/${WebKit2_LIBRARY_NAME}-${PROJECT_VERSION_MAJOR}/themes)
ENDIF ()

IF (ENABLE_TIZEN_DRAG_SUPPORT)
    INSTALL(FILES ${DRAG_THEME}
        DESTINATION share/${WebKit2_LIBRARY_NAME}-${PROJECT_VERSION_MAJOR}/themes)
ENDIF ()

IF (ENABLE_TIZEN_WEBKIT2_FORM_DATABASE)
    INSTALL(FILES ${FORMDATA_THEME}
        DESTINATION share/${WebKit2_LIBRARY_NAME}-${PROJECT_VERSION_MAJOR}/themes)
ENDIF ()

IF (ENABLE_TIZEN_EDGE_SUPPORT)
    INSTALL(FILES ${EDGE_THEME}
        DESTINATION share/${WebKit2_LIBRARY_NAME}-${PROJECT_VERSION_MAJOR}/themes)
ENDIF ()

IF (ENABLE_TIZEN_LINK_MAGNIFIER)
    INSTALL(FILES ${LINK_MAGNIFIER_THEME} DESTINATION share/${WebKit2_LIBRARY_NAME}-${PROJECT_VERSION_MAJOR}/themes)
ENDIF ()

FILE(GLOB InjectedBundle_API_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/WebProcess/InjectedBundle/API/c/*.h")
# FIXME: WKBundleFrame.h includes WKFrame.h
LIST(APPEND InjectedBundle_API_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/UIProcess/API/C/WKFrame.h"
)
INSTALL(FILES ${InjectedBundle_API_HEADERS}
    DESTINATION include/${WebKit2_LIBRARY_NAME}-${PROJECT_VERSION_MAJOR}/WebKit2)

ADD_DEFINITIONS("-DWEBKIT_TEXT_DIR=\"${CMAKE_INSTALL_PREFIX}/share/${WebKit2_LIBRARY_NAME}-${PROJECT_VERSION_MAJOR}/locale/po\"")
ADD_DEFINITIONS("-DWEBKIT_HTML_DIR=\"${CMAKE_INSTALL_PREFIX}/share/${WebKit2_LIBRARY_NAME}-${PROJECT_VERSION_MAJOR}/html\"")
INCLUDE_IF_EXISTS(${WEBKIT2_DIR}/UIProcess/efl/po_tizen/CMakeLists.txt)
SET(WEBKIT2_EFL_ERROR_PAGE_DIR share/${WebKit2_LIBRARY_NAME}-${PROJECT_VERSION_MAJOR}/html)
INSTALL(FILES ${WEBKIT2_DIR}/UIProcess/efl/htmlfiles/errorPage.html DESTINATION ${WEBKIT2_EFL_ERROR_PAGE_DIR})

IF (ENABLE_TIZEN_SCREEN_READER)
    SET(SCREEN_READER_FOCUS_RING_IMAGE_PATH ${CMAKE_INSTALL_PREFIX}/share/${WebKit2_LIBRARY_NAME}-${PROJECT_VERSION_MAJOR}/images)
    ADD_DEFINITIONS(-DSCREEN_READER_FOCUS_RING_IMAGE_PATH="${SCREEN_READER_FOCUS_RING_IMAGE_PATH}/screenReaderFocusRing.png")
    INSTALL(FILES ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/images/screenReaderFocusRing.png DESTINATION ${SCREEN_READER_FOCUS_RING_IMAGE_PATH})
ENDIF ()

IF (ENABLE_TIZEN_FOCUS_UI)
    SET(FOCUS_UI_FOCUS_RING_IMAGE_PATH ${CMAKE_INSTALL_PREFIX}/share/${WebKit2_LIBRARY_NAME}-${PROJECT_VERSION_MAJOR}/images)
    ADD_DEFINITIONS(-DFOCUS_UI_FOCUS_RING_IMAGE_PATH="${FOCUS_UI_FOCUS_RING_IMAGE_PATH}/focusUIFocusRing.png")
    INSTALL(FILES ${WEBKIT2_DIR}/UIProcess/API/efl/tizen/images/focusUIFocusRing.png DESTINATION ${FOCUS_UI_FOCUS_RING_IMAGE_PATH})
ENDIF ()

IF (ENABLE_TIZEN_WEBKIT_EXTENSION)
    LIST(APPEND WebKit2StaticForDebug_INCLUDE_DIRECTORIES
        ${WKEXT_INCLUDE_DIRS}
    )
    LIST(APPEND WebKit2_LIBRARIES
        ${WKEXT_LIBRARIES}
    )
ENDIF()

IF (ENABLE_TIZEN_COMPRESSION_PROXY)
    LIST(APPEND WebKit2StaticForDebug_INCLUDE_DIRECTORIES
        ${UCPROXYSDK_INCLUDE_DIRS}
    )
    LIST(APPEND WebKit2_LIBRARIES
        ${UCPROXYSDK_LIBRARIES}
    )
ENDIF ()
