
# ---------------------------------------------------------------------------------
#                       Start make the plugin library (libgstcamerasrc)
# ---------------------------------------------------------------------------------

# plugindir is set in configure
plugin_LTLIBRARIES = libgstcamerasrc.la

libgstcamerasrc_la_CFLAGS := $(X11_CFLAGS)
libgstcamerasrc_la_CFLAGS += $(XFIXES_CFLAGS)
libgstcamerasrc_la_CFLAGS += $(GST_CFLAGS)
libgstcamerasrc_la_CFLAGS += $(GST_BASE_CFLAGS)
libgstcamerasrc_la_CFLAGS += $(GST_VIDEO_FLAGS)
libgstcamerasrc_la_CFLAGS += $(CAMSRCJPEGENC_CFLAGS)
libgstcamerasrc_la_CFLAGS += $(GST_INTERFACES_CFLAGS)
libgstcamerasrc_la_CFLAGS += $(MMUTIL_IMGP_CFLAGS)
libgstcamerasrc_la_CFLAGS += $(MMUTIL_JPEG_CFLAGS)
libgstcamerasrc_la_CFLAGS += $(MM_COMMON_CFLAGS)
libgstcamerasrc_la_CFLAGS += $(GLIB_CFLAGS)
libgstcamerasrc_la_CFLAGS += -I$(srcdir)/include
libgstcamerasrc_la_CFLAGS += -I$(srcdir)/include/HAL/ss

libgstcamerasrc_la_CFLAGS += -DCAMERA_INTF
libgstcamerasrc_la_CFLAGS += -DCAMERA_ION_FALLBACK_HEAP_ID=ION_IOMMU_HEAP_ID
libgstcamerasrc_la_CFLAGS += -DCAMERA_ION_HEAP_ID=ION_IOMMU_HEAP_ID
libgstcamerasrc_la_CFLAGS += -DString8=char*
libgstcamerasrc_la_CFLAGS += -D__PLATFORM_TIZEN__
libgstcamerasrc_la_CFLAGS += -DCONFIG_AUTO_LLS_NV21
libgstcamerasrc_la_CFLAGS += -DCONFIG_MACH_Z3LTE
libgstcamerasrc_la_CFLAGS += -DDISABLE_FACEDETECTION
libgstcamerasrc_la_CFLAGS += -DFIXED_RESOLUTION
libgstcamerasrc_la_CFLAGS += -DCONFIG_Q3A_ALGO_AF_DAMPING
libgstcamerasrc_la_CFLAGS += -DSENSOR_S5K4H5YB
libgstcamerasrc_la_CFLAGS += -DFRONT_CAMERA_VTMODE_BINNING
libgstcamerasrc_la_CFLAGS += -DCONFIG_Q3A_ALGO_RB2
libgstcamerasrc_la_CFLAGS += -DCONFIG_Q3A_ALGO_STANDALONE
libgstcamerasrc_la_CFLAGS += -DCONFIG_Q3A_ALGO_AF_DAMPING
libgstcamerasrc_la_CFLAGS += -DCONFIG_Q3A_ALGO_AWB_AL
libgstcamerasrc_la_CFLAGS += -DDEFAULT_ZSL_MODE_ON
if IS_DEBUG
libgstcamerasrc_la_CFLAGS += -DLOG_DEBUG
endif
if MSM8X30
libgstcamerasrc_la_CFLAGS += -DMSM_8X30
endif
if MSM8974
libgstcamerasrc_la_CFLAGS += -DMSM_8974
endif

libgstcamerasrc_la_CPPFLAGS = -I$(srcdir)/include
libgstcamerasrc_la_CPPFLAGS += -I$(srcdir)/include/HAL/ss
libgstcamerasrc_la_CPPFLAGS += -I/usr/include/mm_camera_interface
libgstcamerasrc_la_CPPFLAGS += -I/usr/include/dlog
libgstcamerasrc_la_CPPFLAGS += -I/usr/include/sensor
libgstcamerasrc_la_CPPFLAGS += $(GLIB_CFLAGS)
libgstcamerasrc_la_CPPFLAGS += -D__PLATFORM_TIZEN__
libgstcamerasrc_la_CPPFLAGS += -DSAMSUNG_CAMERA
libgstcamerasrc_la_CPPFLAGS += -DCONFIG_AUTO_LLS_NV21
#libgstcamerasrc_la_CPPFLAGS += -DCAMERA_SEPARATE_MOVIE_FLASH
#libgstcamerasrc_la_CPPFLAGS += -DREAR_CAMERA_8M
libgstcamerasrc_la_CPPFLAGS += -DCONFIG_MACH_Z3LTE
libgstcamerasrc_la_CPPFLAGS += -DDISABLE_FACEDETECTION
libgstcamerasrc_la_CPPFLAGS += -DFIXED_RESOLUTION
libgstcamerasrc_la_CPPFLAGS += -DCONFIG_Q3A_ALGO_AF_DAMPING
libgstcamerasrc_la_CPPFLAGS += -DSENSOR_S5K4H5YB
libgstcamerasrc_la_CPPFLAGS += -DFRONT_CAMERA_VTMODE_BINNING
libgstcamerasrc_la_CPPFLAGS += -DCONFIG_Q3A_ALGO_RB2
libgstcamerasrc_la_CPPFLAGS += -DCONFIG_Q3A_ALGO_STANDALONE
libgstcamerasrc_la_CPPFLAGS += -DCONFIG_Q3A_ALGO_AF_DAMPING
libgstcamerasrc_la_CPPFLAGS += -DCONFIG_Q3A_ALGO_AWB_AL
libgstcamerasrc_la_CPPFLAGS += -Dstrlcpy=g_strlcpy
if MSM8974
libgstcamerasrc_la_CPPFLAGS += -DUSE_TBM
endif


# sources used to compile this plug-in
libgstcamerasrc_la_SOURCES := gstcamerasrccontrol.c
libgstcamerasrc_la_SOURCES += gstcamerasrccolorbalance.c
libgstcamerasrc_la_SOURCES += gstqcamerasrc.c
libgstcamerasrc_la_SOURCES += HAL/ss/qcamera_main.c
libgstcamerasrc_la_SOURCES += HAL/ss/qcamera_preview.c
libgstcamerasrc_la_SOURCES += HAL/ss/qcamera_snapshot.c
libgstcamerasrc_la_SOURCES += HAL/ss/qcamera_record.c

libgstcamerasrc_la_LIBADD = $(X11_LIBS) \
                            $(XFIXES_LIBS) \
                            $(GST_LIBS) \
                            $(GST_BASE_LIBS) \
                            $(GST_PLUGINS_BASE_LIBS) \
                            $(GST_VIDEO_LIBS) \
                            -ldl -lpthread -lrt -lm \
                            $(CAMSRCJPEGENC_LIBS) \
                            $(GST_INTERFACES_LIBS) \
                            $(MMUTIL_IMGP_LIBS) \
                            $(MMUTIL_JPEG_LIBS) \
                            $(GLIB_LIBS) \
                            -lqcamhal \
                            -lcamerahdr


if MSM8X30
libgstcamerasrc_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) -limageomxcore -lm -L./libs
endif
if MSM8974
libgstcamerasrc_la_LDFLAGS = $(X11_LDFLAGS) $(XFIXES_LDFLAGS) $(GST_PLUGIN_LDFLAGS) -lm -L./libs 
endif

# ---------------------------------------------------------------------------------
#                       End make the shared library (libgstcamerasrc)
# ---------------------------------------------------------------------------------
install-exec-hook:
	mkdir -p $(DESTDIR)$(prefix)/lib && \
	cp -P $(srcdir)/libs/* $(DESTDIR)$(prefix)/lib/
