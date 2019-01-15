# plugindir is set in configure

##############################################################################
# change libgstplugin.la to something more suitable, e.g. libmysomething.la  #
##############################################################################
plugin_LTLIBRARIES = libgstpip.la

##############################################################################
# for the next set of variables, rename the prefix if you renamed the .la,   #
#  e.g. libgstplugin_la_SOURCES => libmysomething_la_SOURCES                 #
#       libgstplugin_la_CFLAGS  => libmysomething_la_CFLAGS                  #
#       libgstplugin_la_LIBADD  => libmysomething_la_LIBADD                  #
#       libgstplugin_la_LDFLAGS => libmysomething_la_LDFLAGS                 #
##############################################################################

# sources used to compile this plug-in
libgstpip_la_SOURCES = gstpip.c texture.c

# flags used to compile this plugin
# add other _CFLAGS and _LIBS as needed
libgstpip_la_CFLAGS = $(GST_CFLAGS)  $(GST_PLUGINS_BASE_CFLAGS) $(GST_BASE_CFLAGS) $(TBM_CFLAGS) $(MM_UTIL_JPEG_CFLAGS) $(DRM_CFLAGS) $(OPENGLES20_CFLAGS) $(XFIXES_CFLAGS) $(DRI2_CFLAGS) $(XPROTO_CFLAGS) $(IMAGE_FILTER_CFLAGS)

libgstpip_la_LIBADD = $(GST_LIBS) $(GST_PLUGINS_BASE_LIBS)  $(GST_BASE_LIBS) $(TBM_LIBS) $(MM_UTIL_JPEG_LIBS) $(DRM_LIBS) $(OPENGLES20_LIBS) $(XFIXES_LIBS) $(DRI2_LIBS) $(XPROTO_LIBS) $(IMAGE_FILTER_LIBS)
libgstpip_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

# headers we need but don't want installed
noinst_HEADERS = gstpip.h texture.h pip-define.h
