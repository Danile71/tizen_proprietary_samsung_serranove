LIST(APPEND WebCore_INCLUDE_DIRECTORIES
  "${WTF_DIR}/wtf/gobject"
  "${WEBCORE_DIR}/accessibility/efl"
  "${WEBCORE_DIR}/page/efl"
  "${WEBCORE_DIR}/platform/efl"
  "${WEBCORE_DIR}/platform/graphics/efl"
  "${WEBCORE_DIR}/platform/graphics/surfaces"
  "${WEBCORE_DIR}/platform/linux"
  "${WEBCORE_DIR}/platform/mediastream/gstreamer"
  "${WEBCORE_DIR}/platform/network/soup"
  "${WEBCORE_DIR}/platform/text/efl"
  "${WEBCORE_DIR}/plugins/efl"
  "${WEBKIT_DIR}/efl/WebCoreSupport"
  "${WEBKIT_DIR}/efl/ewk"
)

LIST(APPEND WebCore_SOURCES
  bindings/js/ScriptControllerEfl.cpp

  page/efl/DragControllerEfl.cpp
  page/efl/EventHandlerEfl.cpp

  platform/Cursor.cpp

  platform/audio/efl/AudioBusEfl.cpp
  platform/audio/gstreamer/AudioDestinationGStreamer.cpp
  platform/audio/gstreamer/AudioFileReaderGStreamer.cpp
  platform/audio/gstreamer/WebKitWebAudioSourceGStreamer.cpp

  platform/efl/BatteryProviderEfl.cpp
  platform/efl/ClipboardEfl.cpp
  platform/efl/ColorChooserEfl.cpp
  platform/efl/ContextMenuEfl.cpp
  platform/efl/ContextMenuItemEfl.cpp
  platform/efl/CursorEfl.cpp
  platform/efl/DragDataEfl.cpp
  platform/efl/DragImageEfl.cpp
  platform/efl/EflKeyboardUtilities.cpp
  platform/efl/EflScreenUtilities.cpp
  platform/efl/ErrorsEfl.cpp
  platform/efl/EventLoopEfl.cpp
  platform/efl/FileSystemEfl.cpp
  platform/efl/GamepadsEfl.cpp
  platform/efl/LanguageEfl.cpp
  platform/efl/LocalizedStringsEfl.cpp
  platform/efl/LoggingEfl.cpp
  platform/efl/MIMETypeRegistryEfl.cpp
  platform/efl/NetworkInfoProviderEfl.cpp
  platform/efl/PasteboardEfl.cpp
  platform/efl/PlatformKeyboardEventEfl.cpp
  platform/efl/PlatformMouseEventEfl.cpp
  platform/efl/PlatformScreenEfl.cpp
  platform/efl/PlatformWheelEventEfl.cpp
  platform/efl/RefPtrEfl.cpp
  platform/efl/RenderThemeEfl.cpp
  platform/efl/RunLoopEfl.cpp
  platform/efl/ScrollViewEfl.cpp
  platform/efl/ScrollbarEfl.cpp
  platform/efl/ScrollbarThemeEfl.cpp
  platform/efl/SharedBufferEfl.cpp
  platform/efl/SharedTimerEfl.cpp
  platform/efl/SoundEfl.cpp
  platform/efl/SystemTimeEfl.cpp
  platform/efl/TemporaryLinkStubs.cpp
  platform/efl/WidgetEfl.cpp

  platform/graphics/ImageSource.cpp
  platform/graphics/efl/CairoUtilitiesEfl.cpp
  platform/graphics/efl/IconEfl.cpp
  platform/graphics/efl/ImageEfl.cpp
  platform/graphics/efl/IntPointEfl.cpp
  platform/graphics/efl/IntRectEfl.cpp

  platform/image-decoders/ImageDecoder.cpp
  platform/image-decoders/AsyncImageDecoder.cpp
  platform/image-decoders/bmp/BMPImageDecoder.cpp
  platform/image-decoders/bmp/BMPImageReader.cpp
  platform/image-decoders/gif/GIFImageDecoder.cpp
  platform/image-decoders/gif/GIFImageReader.cpp
  platform/image-decoders/ico/ICOImageDecoder.cpp
  platform/image-decoders/jpeg/JPEGImageDecoder.cpp
  platform/image-decoders/png/PNGImageDecoder.cpp
  platform/image-decoders/webp/WEBPImageDecoder.cpp

  platform/linux/GamepadDeviceLinux.cpp

  platform/mediastream/gstreamer/MediaStreamCenterGStreamer.cpp

  platform/network/soup/CookieJarSoup.cpp
  platform/network/soup/CookieStorageSoup.cpp
  platform/network/soup/CredentialStorageSoup.cpp
  platform/network/soup/DNSSoup.cpp
  platform/network/soup/GOwnPtrSoup.cpp
  platform/network/soup/ProxyResolverSoup.cpp
  platform/network/soup/ProxyServerSoup.cpp
  platform/network/soup/ResourceHandleSoup.cpp
  platform/network/soup/ResourceRequestSoup.cpp
  platform/network/soup/ResourceResponseSoup.cpp
  platform/network/soup/SocketStreamHandleSoup.cpp
  platform/network/soup/SoupURIUtils.cpp

  platform/PlatformStrategies.cpp

  platform/posix/FileSystemPOSIX.cpp

  platform/text/efl/TextBreakIteratorInternalICUEfl.cpp
  platform/text/enchant/TextCheckerEnchant.cpp
)

IF (ENABLE_NETSCAPE_PLUGIN_API)
  LIST(APPEND WebCore_SOURCES
    plugins/PluginDatabase.cpp
    plugins/PluginDebug.cpp
    plugins/PluginPackage.cpp
    plugins/PluginStream.cpp
    plugins/PluginView.cpp

    plugins/efl/PluginPackageEfl.cpp
    plugins/efl/PluginViewEfl.cpp
  )
ELSE ()
  LIST(APPEND WebCore_SOURCES
    plugins/PluginPackageNone.cpp
    plugins/PluginViewNone.cpp
  )
ENDIF ()

LIST(APPEND WebCore_USER_AGENT_STYLE_SHEETS
    ${WEBCORE_DIR}/css/mediaControlsEfl.css
    ${WEBCORE_DIR}/css/mediaControlsEflFullscreen.css
)

IF (WTF_USE_CAIRO)
  LIST(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/platform/cairo"
    "${WEBCORE_DIR}/platform/graphics/cairo"
    "${WEBCORE_DIR}/platform/graphics/surfaces"

  )
  LIST(APPEND WebCore_SOURCES
    platform/cairo/WidgetBackingStoreCairo.cpp
    platform/graphics/cairo/BitmapImageCairo.cpp
    platform/graphics/cairo/CairoUtilities.cpp
    platform/graphics/cairo/FontCairo.cpp
    platform/graphics/cairo/GradientCairo.cpp
    platform/graphics/cairo/GraphicsContextCairo.cpp
    platform/graphics/cairo/ImageBufferCairo.cpp
    platform/graphics/cairo/ImageCairo.cpp
    platform/graphics/cairo/IntRectCairo.cpp
    platform/graphics/cairo/NativeImageCairo.cpp
    platform/graphics/cairo/OwnPtrCairo.cpp
    platform/graphics/cairo/PathCairo.cpp
    platform/graphics/cairo/PatternCairo.cpp
    platform/graphics/cairo/PlatformContextCairo.cpp
    platform/graphics/cairo/PlatformPathCairo.cpp
    platform/graphics/cairo/RefPtrCairo.cpp
    platform/graphics/cairo/TileCairo.cpp
    platform/graphics/cairo/TiledBackingStoreBackendCairo.cpp
    platform/graphics/cairo/TransformationMatrixCairo.cpp

    platform/image-decoders/cairo/ImageDecoderCairo.cpp
  )

  IF (WTF_USE_FREETYPE)
    LIST(APPEND WebCore_INCLUDE_DIRECTORIES
      "${WEBCORE_DIR}/platform/graphics/freetype"
      "${WEBCORE_DIR}/platform/graphics/harfbuzz/"
      "${WEBCORE_DIR}/platform/graphics/harfbuzz/ng"
      ${HARFBUZZ_INCLUDE_DIRS}
    )
    LIST(APPEND WebCore_SOURCES
      platform/graphics/WOFFFileFormat.cpp
      platform/graphics/cairo/FontCairoHarfbuzzNG.cpp
      platform/graphics/freetype/FontCacheFreeType.cpp
      platform/graphics/freetype/FontCustomPlatformDataFreeType.cpp
      platform/graphics/freetype/FontPlatformDataFreeType.cpp
      platform/graphics/freetype/GlyphPageTreeNodeFreeType.cpp
      platform/graphics/freetype/SimpleFontDataFreeType.cpp
      platform/graphics/harfbuzz/HarfBuzzShaperBase.cpp
      platform/graphics/harfbuzz/ng/HarfBuzzNGFace.cpp
      platform/graphics/harfbuzz/ng/HarfBuzzNGFaceCairo.cpp
      platform/graphics/harfbuzz/ng/HarfBuzzShaper.cpp
    )
    LIST(APPEND WebCore_LIBRARIES
      ${HARFBUZZ_LIBRARIES}
    )
  ENDIF ()

  IF (WTF_USE_PANGO)
    LIST(APPEND WebCore_INCLUDE_DIRECTORIES
      "${WEBCORE_DIR}/platform/graphics/pango"
      ${Pango_INCLUDE_DIRS}
    )
    LIST(APPEND WebCore_SOURCES
      platform/graphics/pango/FontPango.cpp
      platform/graphics/pango/FontCachePango.cpp
      platform/graphics/pango/FontCustomPlatformDataPango.cpp
      platform/graphics/pango/FontPlatformDataPango.cpp
      platform/graphics/pango/GlyphPageTreeNodePango.cpp
      platform/graphics/pango/SimpleFontDataPango.cpp
      platform/graphics/pango/PangoUtilities.cpp
    )
    LIST(APPEND WebCore_LIBRARIES
      ${Pango_LIBRARY}
      ${Pango_Cairo_LIBRARY}
    )
  ENDIF ()
ENDIF ()

IF (WTF_USE_SKIA)
  LIST(APPEND WebCore_INCLUDE_DIRECTORIES
    ${Skia_INCLUDE_DIRS}
    ${Harfbuzz_INCLUDE_DIRS}
    "${WEBCORE_DIR}/platform/graphics/skia"
    "${WEBCORE_DIR}/platform/image-encoders/skia"
    "${WEBCORE_DIR}/platform/chromium"
    "${WEBCORE_DIR}/platform/graphics/chromium"
    "${WEBCORE_DIR}/platform/graphics/gpu"
    "${WEBCORE_DIR}/platform/graphics/opentype"
  )
  LIST(APPEND WebCore_SOURCES
    platform/graphics/GraphicsContext3D.cpp
    platform/graphics/chromium/HarfbuzzSkia.cpp
    platform/graphics/chromium/FontLinux.cpp
    platform/graphics/chromium/FontCacheLinux.cpp
    platform/graphics/chromium/SimpleFontDataLinux.cpp
    platform/graphics/chromium/FontPlatformDataLinux.cpp
    platform/graphics/chromium/ImageChromium.cpp
    platform/graphics/chromium/VDMXParser.cpp
    platform/graphics/chromium/VDMXParser.h
    platform/graphics/gpu/DrawingBuffer.cpp
    platform/graphics/gpu/Texture.cpp
    platform/graphics/gpu/TilingData.cpp
    platform/graphics/skia/base/message_pump_glib.cc
    platform/graphics/skia/base/message_pump_libevent.cc
    platform/graphics/skia/base/message_pump.cc
    platform/graphics/skia/base/message_pump_default.cc
    platform/graphics/skia/base/task.cc
    platform/graphics/skia/base/lazy_instance.cc
    platform/graphics/skia/base/thread_local_posix.cc
    platform/graphics/skia/base/non_thread_safe.cc
    platform/graphics/skia/base/thread_checker.cc
    platform/graphics/skia/base/tracked.cc
    platform/graphics/skia/base/weak_ptr.cc
    platform/graphics/skia/base/tracked_objects.cc
    platform/graphics/skia/base/thread_local_storage_posix.cc
    platform/graphics/skia/base/condition_variable_posix.cc
    platform/graphics/skia/base/message_loop.cc
    platform/graphics/skia/base/waitable_event_posix.cc
    platform/graphics/skia/base/file_util_posix.cc
    platform/graphics/skia/base/debug_util_posix.cc
    platform/graphics/skia/base/process_util_posix.cc
    platform/graphics/skia/base/time_posix.cc
    platform/graphics/skia/base/sys_info_posix.cc
    platform/graphics/skia/base/at_exit.cc
    platform/graphics/skia/base/base_switches.cc
    platform/graphics/skia/base/command_line.cc
    platform/graphics/skia/base/debug_util.cc
    platform/graphics/skia/base/file_descriptor_shuffle.cc
    platform/graphics/skia/base/file_path.cc
    platform/graphics/skia/base/file_util.cc
    platform/graphics/skia/base/file_util_linux.cc
    platform/graphics/skia/base/histogram.cc
    platform/graphics/skia/base/lock.cc
    platform/graphics/skia/base/lock_impl_posix.cc
    platform/graphics/skia/base/logging.cc
    platform/graphics/skia/base/pickle.cc
    platform/graphics/skia/base/platform_file_posix.cc
    platform/graphics/skia/base/platform_thread_posix.cc
    platform/graphics/skia/base/process_linux.cc
    platform/graphics/skia/base/process_posix.cc
    platform/graphics/skia/base/process_util.cc
    platform/graphics/skia/base/process_util_linux.cc
    platform/graphics/skia/base/ref_counted.cc
    platform/graphics/skia/base/safe_strerror_posix.cc
    platform/graphics/skia/base/setproctitle_linux.c
    platform/graphics/skia/base/string16.cc
    platform/graphics/skia/base/string_number_conversions.cc
    platform/graphics/skia/base/string_piece.cc
    platform/graphics/skia/base/string_split.cc
    platform/graphics/skia/base/string_util.cc
    platform/graphics/skia/base/stringprintf.cc
    platform/graphics/skia/base/sys_info_linux.cc
    platform/graphics/skia/base/sys_string_conversions_linux.cc
    platform/graphics/skia/base/thread_collision_warner.cc
    platform/graphics/skia/base/time.cc
    platform/graphics/skia/base/unix_domain_socket_posix.cc
    platform/graphics/skia/base/utf_string_conversion_utils.cc
    platform/graphics/skia/base/utf_string_conversions.cc
    platform/graphics/skia/base/vlog.cc
    platform/graphics/skia/base/third_party/dmg_fp/dtoa.cc
    platform/graphics/skia/base/third_party/dmg_fp/g_fmt.cc
    platform/graphics/skia/base/third_party/dynamic_annotations/dynamic_annotations.c
    platform/graphics/skia/base/third_party/icu/icu_utf.cc
    platform/graphics/skia/base/third_party/nspr/prtime.cc
    platform/graphics/skia/ext/SkFontHost_fontconfig.cpp
    platform/graphics/skia/ext/SkFontHost_fontconfig_direct.cpp
    platform/graphics/skia/ext/SkMemory_new_handler.cpp
    platform/graphics/skia/ext/SkThread_chrome.cc
    platform/graphics/skia/ext/SkTypeface_fake.cpp
    platform/graphics/skia/ext/bitmap_platform_device.cc
    platform/graphics/skia/ext/bitmap_platform_device_linux.cc
    platform/graphics/skia/ext/convolver.cc
    platform/graphics/skia/ext/google_logging.cc
    platform/graphics/skia/ext/image_operations.cc
    platform/graphics/skia/ext/platform_canvas.cc
    platform/graphics/skia/ext/platform_canvas_linux.cc
    platform/graphics/skia/ext/platform_device_linux.cc
    platform/graphics/skia/ext/vector_canvas.cc
    platform/graphics/skia/ext/vector_canvas_linux.cc
    platform/graphics/skia/ext/vector_platform_device_linux.cc
    platform/graphics/skia/FontCustomPlatformData.cpp
    platform/graphics/skia/FloatPointSkia.cpp
    platform/graphics/skia/FloatRectSkia.cpp
    platform/graphics/skia/GradientSkia.cpp
    platform/graphics/skia/GraphicsContext3DSkia.cpp
    platform/graphics/skia/GraphicsContextSkia.cpp
    platform/graphics/skia/GlyphPageTreeNodeSkia.cpp
    platform/graphics/skia/ImageBufferSkia.cpp
    platform/graphics/skia/ImageSkia.cpp
    platform/graphics/skia/IntPointSkia.cpp
    platform/graphics/skia/IntRectSkia.cpp
    platform/graphics/skia/NativeImageSkia.cpp
    platform/graphics/skia/PathSkia.cpp
    platform/graphics/skia/PatternSkia.cpp
    platform/graphics/skia/PlatformContextSkia.cpp
    platform/graphics/skia/SkiaUtils.cpp
    platform/graphics/skia/TransformationMatrixSkia.cpp
    platform/graphics/chromium/ComplexTextControllerLinux.cpp
    platform/image-encoders/skia/JPEGImageEncoder.cpp
    platform/image-encoders/skia/PNGImageEncoder.cpp
    platform/image-decoders/skia/ImageDecoderSkia.cpp
  )
    LIST(APPEND WebCore_LIBRARIES
      ${Skia_LIBRARIES}
      ${Harfbuzz_LIBRARIES}
    )
  IF (WTF_USE_FREETYPE)
    LIST(APPEND WebCore_INCLUDE_DIRECTORIES
      "${WEBCORE_DIR}/platform/graphics/freetype"
    )
    LIST(APPEND WebCore_SOURCES
      platform/graphics/WOFFFileFormat.cpp
    )
  ENDIF ()
  IF (WTF_USE_PANGO)
    LIST(APPEND WebCore_INCLUDE_DIRECTORIES
      "${WEBCORE_DIR}/platform/graphics/pango"
      ${Pango_INCLUDE_DIRS}
    )
    LIST(APPEND WebCore_SOURCES
      platform/graphics/pango/FontPango.cpp
      platform/graphics/pango/FontCachePango.cpp
      platform/graphics/pango/FontCustomPlatformDataPango.cpp
      platform/graphics/pango/FontPlatformDataPango.cpp
      platform/graphics/pango/GlyphPageTreeNodePango.cpp
      platform/graphics/pango/SimpleFontDataPango.cpp
      platform/graphics/pango/PangoUtilities.cpp
    )
    LIST(APPEND WebCore_LIBRARIES
      ${Pango_LIBRARY}
      ${Pango_Cairo_LIBRARY}
    )
  ENDIF ()
ENDIF ()

IF (WTF_USE_ICU_UNICODE)
  LIST(APPEND WebCore_SOURCES
    editing/SmartReplaceICU.cpp
    platform/text/TextEncodingDetectorICU.cpp
    platform/text/TextBreakIteratorICU.cpp
    platform/text/TextCodecICU.cpp
  )
ENDIF ()

LIST(APPEND WebCore_LIBRARIES
  ${CAIRO_LIBRARIES}
  ${ECORE_LIBRARIES}
  ${ECORE_EVAS_LIBRARIES}
  ${ECORE_FILE_LIBRARIES}
  ${ECORE_X_LIBRARIES}
  ${E_DBUS_LIBRARIES}
  ${E_DBUS_EUKIT_LIBRARIES}
  ${EDJE_LIBRARIES}
  ${EINA_LIBRARIES}
  ${EVAS_LIBRARIES}
  ${FONTCONFIG_LIBRARIES}
  ${FREETYPE2_LIBRARIES}
  ${ICU_LIBRARIES}
  ${JPEG_LIBRARY}
  ${LIBXML2_LIBRARIES}
  ${LIBXSLT_LIBRARIES}
  ${PNG_LIBRARY}
  ${SQLITE_LIBRARIES}
  ${GLIB_LIBRARIES}
  ${GLIB_GIO_LIBRARIES}
  ${GLIB_GOBJECT_LIBRARIES}
  ${LIBSOUP_LIBRARIES}
  ${ZLIB_LIBRARIES}
)

LIST(APPEND WebCore_INCLUDE_DIRECTORIES
  ${CAIRO_INCLUDE_DIRS}
  ${ECORE_INCLUDE_DIRS}
  ${ECORE_EVAS_INCLUDE_DIRS}
  ${ECORE_FILE_INCLUDE_DIRS}
  ${ECORE_X_INCLUDE_DIRS}
  ${E_DBUS_INCLUDE_DIRS}
  ${E_DBUS_EUKIT_INCLUDE_DIRS}
  ${EDJE_INCLUDE_DIRS}
  ${EINA_INCLUDE_DIRS}
  ${EVAS_INCLUDE_DIRS}
  ${FREETYPE2_INCLUDE_DIRS}
  ${ICU_INCLUDE_DIRS}
  ${LIBXML2_INCLUDE_DIR}
  ${LIBXSLT_INCLUDE_DIR}
  ${SQLITE_INCLUDE_DIR}
  ${GLIB_INCLUDE_DIRS}
  ${LIBSOUP_INCLUDE_DIRS}
  ${ZLIB_INCLUDE_DIRS}
)

IF (ENABLE_VIDEO OR ENABLE_WEB_AUDIO)
  LIST(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/platform/graphics/gstreamer"

    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_BASE_INCLUDE_DIRS}
    ${GSTREAMER_APP_INCLUDE_DIRS}
    ${GSTREAMER_INTERFACES_INCLUDE_DIRS}
    ${GSTREAMER_PBUTILS_INCLUDE_DIRS}
  )
  LIST(APPEND WebCore_SOURCES
    platform/graphics/gstreamer/GRefPtrGStreamer.cpp
    platform/graphics/gstreamer/GStreamerUtilities.cpp
    platform/graphics/gstreamer/GStreamerVersioning.cpp
  )
  LIST(APPEND WebCore_LIBRARIES
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_BASE_LIBRARIES}
    ${GSTREAMER_APP_LIBRARIES}
    ${GSTREAMER_INTERFACES_LIBRARIES}
    ${GSTREAMER_PBUTILS_LIBRARIES}
  )
ENDIF ()

IF (ENABLE_VIDEO)
  LIST(APPEND WebCore_INCLUDE_DIRECTORIES
    ${GSTREAMER_VIDEO_INCLUDE_DIRS}
  )
  LIST(APPEND WebCore_SOURCES
    platform/graphics/gstreamer/GStreamerGWorld.cpp
    platform/graphics/gstreamer/ImageGStreamerCairo.cpp
    platform/graphics/gstreamer/MediaPlayerPrivateGStreamer.cpp
    platform/graphics/gstreamer/PlatformVideoWindowEfl.cpp
    platform/graphics/gstreamer/URIUtils.cpp
    platform/graphics/gstreamer/VideoSinkGStreamer.cpp
    platform/graphics/gstreamer/WebKitWebSourceGStreamer.cpp
  )
  LIST(APPEND WebCore_LIBRARIES
    ${GSTREAMER_VIDEO_LIBRARIES}
  )
ENDIF ()

IF (WTF_USE_3D_GRAPHICS)
  SET(WTF_USE_OPENGL 1)
  ADD_DEFINITIONS(-DWTF_USE_OPENGL=1)

  LIST(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/platform/graphics/cairo"
    "${WEBCORE_DIR}/platform/graphics/opengl"
    "${WEBCORE_DIR}/platform/graphics/texmap"
  )
  LIST(APPEND WebCore_LIBRARIES
    ${OPENGL_gl_LIBRARY}
  )
  LIST(APPEND WebCore_SOURCES
    platform/graphics/OpenGLShims.cpp
    platform/graphics/cairo/DrawingBufferCairo.cpp
    platform/graphics/cairo/GLContext.cpp
    platform/graphics/cairo/GraphicsContext3DCairo.cpp
    platform/graphics/cairo/GraphicsContext3DPrivate.cpp
    platform/graphics/opengl/Extensions3DOpenGL.cpp
    platform/graphics/opengl/Extensions3DOpenGLCommon.cpp
    platform/graphics/opengl/GraphicsContext3DOpenGL.cpp
    platform/graphics/opengl/GraphicsContext3DOpenGLCommon.cpp
    platform/graphics/texmap/TextureMapperGL.cpp
    platform/graphics/texmap/TextureMapperShaderManager.cpp
  )
ENDIF ()

#ADD_DEFINITIONS(-DWTF_USE_CROSS_PLATFORM_CONTEXT_MENUS=1
ADD_DEFINITIONS(-DWTF_USE_CROSS_PLATFORM_CONTEXT_MENUS=0
                -DDATA_DIR="${CMAKE_INSTALL_PREFIX}/${DATA_INSTALL_DIR}")

SET(IMAGE_RESOURCE_DIR ${CMAKE_INSTALL_PREFIX}/${DATA_INSTALL_DIR}/images)
INSTALL(FILES ${WEBCORE_DIR}/Resources/efl/missingImage.png DESTINATION "${IMAGE_RESOURCE_DIR}")
INSTALL(FILES ${WEBCORE_DIR}/Resources/efl/missingImage@2x.png DESTINATION "${IMAGE_RESOURCE_DIR}")

IF (ENABLE_GAMEPAD)
  LIST(APPEND WebCore_INCLUDE_DIRECTORIES
    ${EEZE_INCLUDE_DIRS}
  )
  LIST(APPEND WebCore_LIBRARIES
    ${EEZE_LIBRARIES}
  )
ENDIF ()

IF (ENABLE_TIZEN_SUPPORT)
    INCLUDE_IF_EXISTS(${WEBCORE_DIR}/PlatformTizen.cmake)
ENDIF ()

IF (ENABLE_SPEECH_SYNTHESIS)
    LIST(APPEND WebCore_SOURCES
        platform/efl/PlatformSpeechSynthesizerEfl.cpp
        platform/efl/tizen/PlatformSynthesisProviderTizen.cpp
    )
    LIST(APPEND WebCore_INCLUDE_DIRECTORIES
        ${TTS_INCLUDE_DIR}
    )
    LIST(APPEND WebCore_LIBRARIES
        ${TTS_LIBRARY}
    )
ENDIF ()
