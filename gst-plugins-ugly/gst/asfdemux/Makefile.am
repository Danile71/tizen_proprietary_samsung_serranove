plugin_LTLIBRARIES = libgstasf.la

libgstasf_la_SOURCES = gstasfdemux.c gstasf.c asfheaders.c asfpacket.c gstrtpasfdepay.c gstrtspwms.c
if USE_DRM_DECRYPTION
libgstasf_la_SOURCES += drm_util_pr.c
endif

libgstasf_la_CFLAGS = $(GST_PLUGINS_BASE_CFLAGS) $(GST_BASE_CFLAGS) $(GST_CFLAGS)
if USE_DRM_DECRYPTION
libgstasf_la_CFLAGS += $(DRM_CLIENT_CFLAGS) $(DRM_TRUSTED_CFLAGS) -DDRM_ENABLE
if FEATURE_SECURE_PLAYBACK_TZ
libgstasf_la_CFLAGS += -DFEATURE_SECURE_PLAYBACK_TZ
endif
endif

libgstasf_la_LIBADD = $(GST_PLUGINS_BASE_LIBS) \
		-lgstriff-@GST_API_VERSION@ -lgstrtsp-@GST_API_VERSION@ -lgstsdp-@GST_API_VERSION@ \
		-lgstrtp-@GST_API_VERSION@ -lgstaudio-@GST_API_VERSION@ -lgsttag-@GST_API_VERSION@ \
		$(GST_BASE_LIBS) $(GST_LIBS) \
		$(WIN32_LIBS)
if USE_DRM_DECRYPTION
libgstasf_la_LIBADD += $(DRM_CLIENT_LIBS) $(DRM_TRUSTED_LIBS)
endif
libgstasf_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstasf_la_LIBTOOLFLAGS = $(GST_PLUGIN_LIBTOOLFLAGS)

noinst_HEADERS = gstasfdemux.h asfheaders.h asfpacket.h gstrtpasfdepay.h gstrtspwms.h
if USE_DRM_DECRYPTION
noinst_HEADERS += drm_util_pr.h
endif

Android.mk: Makefile.am $(BUILT_SOURCES)
	androgenizer \
	-:PROJECT libgstasf -:SHARED libgstasf \
	 -:TAGS eng debug \
         -:REL_TOP $(top_srcdir) -:ABS_TOP $(abs_top_srcdir) \
	 -:SOURCES $(libgstasf_la_SOURCES) \
	 -:CFLAGS $(DEFS) $(DEFAULT_INCLUDES) $(libgstasf_la_CFLAGS) \
	 -:LDFLAGS $(libgstasf_la_LDFLAGS) \
	           $(libgstasf_la_LIBADD) \
	           -ldl \
	 -:PASSTHROUGH LOCAL_ARM_MODE:=arm \
		       LOCAL_MODULE_PATH:='$$(TARGET_OUT)/lib/gstreamer-0.10' \
	> $@
