LIST(APPEND WebCore_INCLUDE_DIRECTORIES
    "${JAVASCRIPTCORE_DIR}/wtf/efl"
    "${WEBCORE_DIR}/Modules/contenthandler"
    "${WEBCORE_DIR}/loader/cache/tizen"
    "${WEBCORE_DIR}/page/efl/tizen"
    "${WEBCORE_DIR}/page/scrolling/coordinatedgraphics"
    "${WEBCORE_DIR}/platform/audio/gstreamer/tizen"
    "${WEBCORE_DIR}/platform/efl/tizen"
    "${WEBCORE_DIR}/platform/graphics/efl/tizen"
    "${WEBCORE_DIR}/platform/graphics/gpu"
    "${WEBCORE_DIR}/platform/graphics/gstreamer/tizen"
    "${WEBCORE_DIR}/platform/graphics/opengl"
    "${WEBCORE_DIR}/platform/graphics/texmap/tizen"
    "${WEBCORE_DIR}/platform/mediastream/tizen"
    "${WEBCORE_DIR}/platform/tizen"
    "${WTF_DIR}/wtf/efl/tizen"
    ${ASM_INCLUDE_DIRS}
    ${CAPI_INCLUDE_DIRS}
    ${CSR_INCLUDE_DIRS}
    ${DEVICED_INCLUDE_DIRS}
    ${ELEMENTARY_INCLUDE_DIRS}
    ${ENCHANT_INCLUDE_DIRS}
    ${GLES20_INCLUDE_DIRS}
    ${GSTREAMER_AUDIO_INCLUDE_DIRS}
    ${GSTREAMER_FFT_INCLUDE_DIRS}
    ${LevelDB_INCLUDE_DIRS}
    ${LevelDB-MemEnv_INCLUDE_DIRS}
    ${MMSOUND_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    ${Pmapi_INCLUDE_DIRS}
    ${VConf_INCLUDE_DIRS}
    ${feedback_INCLUDE_DIR}
    ${TBM_INCLUDE_DIRS}
    ${DRI2_INCLUDE_DIRS}
    ${UTILX_INCLUDE_DIRS}
    ${DRM_INCLUDE_DIRS}
    ${EFL_ASSIST_INCLUDE_DIRS}
)

LIST(APPEND WebCore_LIBRARIES
    ${ASM_LIBRARIES}
    ${CAPI_LIBRARIES}
    ${CSR_LIBRARIES}
    ${DEVICED_LIBRARIES}
    ${ELEMENTARY_LIBRARIES}
    ${ENCHANT_LIBRARIES}
    ${GLES20_LIBRARIES}
    ${GSTREAMER_AUDIO_LIBRARIES}
    ${GSTREAMER_FFT_LIBRARIES}
    ${LevelDB_LIBRARIES}
    ${LevelDB-MemEnv_LIBRARIES}
    ${MMSOUND_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${Pmapi_LIBRARIES}
    ${VConf_LIBRARIES}
    ${TBM_LIBRARIES}
    ${DRI2_LIBRARIES}
    ${UTILX_LIBRARIES}
    ${DRM_LIBRARIES}
    ${EFL_ASSIST_LIBRARIES}
)

LIST(APPEND WebCore_USER_AGENT_STYLE_SHEETS
    ${WEBCORE_DIR}/css/mediaControlsTizen.css
    ${WEBCORE_DIR}/css/mediaControlsTizenFullscreenHorizontal.css
    ${WEBCORE_DIR}/css/mediaControlsTizenFullscreenVertical.css
)

# Replace EFL port files with Tizen's.
LIST(REMOVE_ITEM WebCore_SOURCES
    platform/efl/ClipboardEfl.cpp
    platform/efl/DragDataEfl.cpp
    platform/efl/DragImageEfl.cpp
    platform/efl/LocalizedStringsEfl.cpp
    platform/efl/NetworkInfoProviderEfl.cpp
    platform/efl/PasteboardEfl.cpp
    platform/efl/ScrollbarThemeEfl.cpp
    platform/mediastream/gstreamer/MediaStreamCenterGStreamer.cpp
    platform/network/soup/ResourceHandleSoup.cpp
    platform/network/soup/SocketStreamHandleSoup.cpp
)

IF (WTF_USE_3D_GRAPHICS)
LIST(REMOVE_ITEM WebCore_SOURCES
    platform/graphics/cairo/DrawingBufferCairo.cpp
    platform/graphics/cairo/GLContext.cpp
    platform/graphics/cairo/GraphicsContext3DCairo.cpp
    platform/graphics/cairo/GraphicsContext3DPrivate.cpp
    platform/graphics/opengl/Extensions3DOpenGL.cpp
    platform/graphics/opengl/Extensions3DOpenGLCommon.cpp
    platform/graphics/opengl/GraphicsContext3DOpenGL.cpp
    platform/graphics/opengl/GraphicsContext3DOpenGLCommon.cpp
)
ENDIF ()

IF (WTF_USE_SKIA)
LIST(REMOVE_ITEM WebCore_SOURCES platform/graphics/efl/FontEfl.cpp)
ENDIF ()

# Begin to add tizen specific files.
LIST(APPEND WebCore_IDL_FILES
    Modules/contenthandler/NavigatorRegisterContentHandler.idl

    Modules/mediastream/BlobCallback.idl

    Modules/mediastream/MediaStreamRecorder.idl

    Modules/speech/DOMWindowSpeechSynthesis.idl

    page/efl/tizen/External.idl
)

LIST(APPEND WebCore_SOURCES
    Modules/contenthandler/NavigatorRegisterContentHandler.cpp

    Modules/mediastream/MediaStreamRecorder.cpp

    page/efl/tizen/External.cpp
    page/scrolling/coordinatedgraphics/ScrollingCoordinatorCoordinatedGraphics.cpp

    platform/audio/ffmpeg/FFTFrameFFMPEG.cpp
    platform/audio/gstreamer/FFTFrameGStreamer.cpp
    platform/audio/gstreamer/tizen/AudioSessionManagerGStreamerTizen.cpp

    platform/efl/tizen/AsyncFileSystemCallbacksTizen.cpp
    platform/efl/tizen/AsyncFileSystemTizen.cpp
    platform/efl/tizen/AsyncFileSystemTaskControllerTizen.cpp
    platform/efl/tizen/AsyncFileWriterClientTizen.cpp
    platform/efl/tizen/AsyncFileWriterTizen.cpp
    platform/efl/tizen/ClipboardTizen.cpp
    platform/efl/tizen/DataObjectTizen.cpp
    platform/efl/tizen/DeviceMotionClientTizen.cpp
    platform/efl/tizen/DeviceOrientationClientTizen.cpp
    platform/efl/tizen/DeviceOrientationProviderTizen.cpp
    platform/efl/tizen/DeviceMotionProviderTizen.cpp
    platform/efl/tizen/DragDataTizen.cpp
    platform/efl/tizen/DragImageTizen.cpp
    platform/efl/tizen/FileSystemTizen.cpp
    platform/efl/tizen/LocalizedStringsTizen.cpp
    platform/efl/tizen/PasteboardTizen.cpp
    platform/efl/tizen/ScrollbarThemeTizen.cpp
    platform/efl/tizen/SSLKeyGeneratorTizen.cpp
    platform/efl/tizen/SSLPrivateKeyStoreTizen.cpp
    platform/efl/tizen/TizenColorUtilities.cpp
    platform/efl/tizen/TizenLinkEffect.cpp
    platform/efl/tizen/TizenExtensibleAPI.cpp
    platform/efl/tizen/WebContentsScanTizen.cpp

    platform/graphics/efl/tizen/Canvas2DLayerTizen.cpp
    platform/graphics/efl/tizen/Extensions3DTizen.cpp
    platform/graphics/efl/tizen/GraphicsContext3D.cpp
    platform/graphics/efl/tizen/GraphicsContext3DInternal.cpp
    platform/graphics/efl/tizen/GraphicsContext3DOffscreen.cpp
    platform/graphics/efl/tizen/SharedPlatformSurfaceTizen.cpp
    platform/graphics/efl/tizen/TizenGraphicsContext3DEfl.cpp
    platform/graphics/gpu/DrawingBuffer.cpp
    platform/graphics/gpu/tizen/DrawingBufferTizen.cpp
    platform/graphics/gstreamer/tizen/MediaResourceControllerGStreamerTizen.cpp
    platform/graphics/gstreamer/tizen/SharedVideoPlatformSurfaceTizen.cpp
    platform/graphics/gstreamer/tizen/VideoLayerTizen.cpp
    platform/graphics/gstreamer/tizen/WebKitCameraSourceGStreamerTizen.cpp
    platform/graphics/surfaces/GraphicsSurface.cpp
    platform/graphics/surfaces/tizen/GraphicsSurfaceTizen.cpp
    platform/graphics/texmap/tizen/PlatformSurfaceTextureGL.cpp

    platform/mediastream/tizen/LocalMediaServer.cpp
    platform/mediastream/tizen/MediaRecorder.cpp
    platform/mediastream/tizen/MediaRecorderPrivateImpl.cpp
    platform/mediastream/tizen/MediaStreamCenterTizen.cpp
    platform/mediastream/tizen/MediaStreamManager.cpp

    platform/network/soup/tizen/ResourceHandleSoupTizen.cpp
    platform/network/soup/tizen/SocketStreamHandleSoupTizen.cpp
    platform/network/tizen/NetworkStateNotifierTizen.cpp

    plugins/PluginPackage.cpp

    platform/tizen/BufferDatabaseTizen.cpp
    loader/cache/tizen/AsyncResourceLoaderTizen.cpp
)

IF (ENABLE_TIZEN_COMPRESSION_PROXY)
LIST(APPEND WebCore_SOURCES
    platform/network/soup/tizen/uc/NetTransactionImpl.cpp
    platform/network/soup/tizen/uc/NetTransactionSoup.cpp
    platform/network/soup/tizen/uc/CookieStorageAccessorImpl.cpp
    platform/network/soup/tizen/uc/HTTPCacheQuerierImpl.cpp
    platform/network/soup/tizen/uc/ProxyDataStreamQuerierImpl.cpp
    platform/network/soup/tizen/uc/ResourceHandleUCProxyTizen.cpp
)
ENDIF ()

IF (ENABLE_ACCESSIBILITY)
    LIST(APPEND WebCore_SOURCES
        accessibility/atk/AXObjectCacheAtk.cpp
        accessibility/atk/AccessibilityObjectAtk.cpp
        accessibility/atk/WebKitAccessibleHyperlink.cpp
        accessibility/atk/WebKitAccessibleInterfaceAction.cpp
        accessibility/atk/WebKitAccessibleInterfaceComponent.cpp
        accessibility/atk/WebKitAccessibleInterfaceDocument.cpp
        accessibility/atk/WebKitAccessibleInterfaceEditableText.cpp
        accessibility/atk/WebKitAccessibleInterfaceHyperlinkImpl.cpp
        accessibility/atk/WebKitAccessibleInterfaceHypertext.cpp
        accessibility/atk/WebKitAccessibleInterfaceImage.cpp
        accessibility/atk/WebKitAccessibleInterfaceSelection.cpp
        accessibility/atk/WebKitAccessibleInterfaceTable.cpp
        accessibility/atk/WebKitAccessibleInterfaceText.cpp
        accessibility/atk/WebKitAccessibleInterfaceValue.cpp
        accessibility/atk/WebKitAccessibleUtil.cpp
        accessibility/atk/WebKitAccessibleWrapperAtk.cpp

        editing/atk/FrameSelectionAtk.cpp
    )
ENDIF ()

IF (WTF_USE_TEXTURE_MAPPER)
    LIST(APPEND WebCore_SOURCES
        platform/graphics/texmap/GraphicsLayerTextureMapper.cpp
        platform/graphics/texmap/TextureMapperCuller.cpp
    )
ELSE ()
    LIST(APPEND WebCore_SOURCES
        platform/graphics/efl/GraphicsLayerEfl.cpp
    )
ENDIF ()

IF (ENABLE_TOUCH_ADJUSTMENT)
    LIST(APPEND WebCore_SOURCES
        page/TouchAdjustment.cpp
    )
ENDIF ()

INCLUDE_IF_EXISTS(${WEBCORE_DIR}/PlatformExperimental.cmake)

IF (NOT ENABLE_TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    LIST(APPEND WebCore_SOURCES
        platform/graphics/surfaces/efl/GraphicsSurfaceEfl.cpp
    )
ENDIF ()

IF (ENABLE_ALLINONE)
    LIST(APPEND WebCore_SOURCES
        css/SVGCSSComputedStyleDeclaration.cpp
        css/SVGCSSParser.cpp
        css/SVGCSSStyleSelector.cpp

        inspector/InspectorAllInOne.cpp

        platform/graphics/SVGGlyph.cpp

        rendering/PointerEventsHitRules.cpp
        rendering/style/SVGRenderStyle.cpp
        rendering/style/SVGRenderStyleDefs.cpp
        rendering/svg/RenderSVGAllInOne.cpp

        svg/SVGAllInOne.cpp
        svg/animation/SMILTime.cpp
        svg/animation/SMILTimeContainer.cpp
        svg/animation/SVGSMILElement.cpp
        svg/graphics/SVGImage.cpp
        svg/graphics/SVGImageCache.cpp
        svg/graphics/filters/SVGFEImage.cpp
        svg/graphics/filters/SVGFilter.cpp
        svg/graphics/filters/SVGFilterBuilder.cpp
        svg/properties/SVGAttributeToPropertyMap.cpp
        svg/properties/SVGPathSegListPropertyTearOff.cpp
    )
ENDIF ()

IF (ENABLE_WEB_AUDIO)
    SET(WEB_AUDIO_DIR ${CMAKE_INSTALL_PREFIX}/${DATA_INSTALL_DIR}/webaudio/resources)
    FILE(GLOB WEB_AUDIO_DATA "${WEBCORE_DIR}/platform/audio/resources/*.wav")
    INSTALL(FILES ${WEB_AUDIO_DATA} DESTINATION ${WEB_AUDIO_DIR})
    ADD_DEFINITIONS(-DUNINSTALLED_AUDIO_RESOURCES_DIR="${WEBCORE_DIR}/platform/audio/resources")
ENDIF ()

IF (ENABLE_SCRIPTED_SPEECH)
    LIST(APPEND WebCore_SOURCES
        platform/efl/tizen/SpeechRecognitionProviderTizen.cpp
        platform/efl/tizen/STTProviderTizen.cpp
    )
    LIST(APPEND WebCore_INCLUDE_DIRECTORIES
        ${STT_INCLUDE_DIR}
    )
    LIST(APPEND WebCore_LIBRARIES
        ${STT_LIBRARIES}
    )
ENDIF ()

IF (ENABLE_GAMEPAD AND ENABLE_TIZEN_GAMEPAD)
    LIST(REMOVE_ITEM WebCore_SOURCES
        platform/efl/GamepadsEfl.cpp
    )

    LIST(APPEND WebCore_SOURCES
        Modules/gamepad/GamepadEventController.cpp
        Modules/gamepad/WebKitGamepadEvent.cpp
        platform/efl/tizen/GamepadsProviderTizen.cpp
    )

    LIST(APPEND WebCore_IDL_FILES
        Modules/gamepad/WebKitGamepadEvent.idl
    )
ENDIF ()

IF (ENABLE_ACCESSIBILITY)
    LIST(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/editing/atk"
        "${WEBCORE_DIR}/accessibility/atk"
        ${ATK_INCLUDE_DIRS}
    )
    list(APPEND WebCore_LIBRARIES
        ${ATK_LIBRARIES}
    )
ENDIF ()

IF (ENABLE_TIZEN_COMPRESSION_PROXY)
LIST(APPEND WebCore_LIBRARIES
    ${WEBP_LIBRARIES}
)
ENDIF ()
