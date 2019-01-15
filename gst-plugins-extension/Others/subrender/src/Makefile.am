# plugindir is set in configure

##############################################################################
# change libgstplugin.la to something more suitable, e.g. libmysomething.la  #
##############################################################################
plugin_LTLIBRARIES = libgstsubrender.la
##############################################################################
# for the next set of variables, rename the prefix if you renamed the .la,   #
#  e.g. libgstplugin_la_SOURCES => libmysomething_la_SOURCES                 #
#       libgstplugin_la_CFLAGS  => libmysomething_la_CFLAGS                  #
#       libgstplugin_la_LIBADD  => libmysomething_la_LIBADD                  #
#       libgstplugin_la_LDFLAGS => libmysomething_la_LDFLAGS                 #
##############################################################################

# sources used to compile this plug-in
libgstsubrender_la_SOURCES = gstsubrender.c

# flags used to compile this plugin
# add other _CFLAGS and _LIBS as needed
libgstsubrender_la_CFLAGS = $(GST_CFLAGS) \
							$(GST_BASE_CFLAGS) \
							$(GSTPB_BASE_CFLAGS) \
							$(PANGO_CFLAGS) \
							$(TBM_CFLAGS) \
							$(DRI2_CFLAGS) \
							$(DRI2PROTO_CFLAGS) \
							$(X_CFLAGS) \
							$(XFIXES_CFLAGS) \
							$(X11_CFLAGS) \
							$(XV_CFLAGS) \
							$(DRM_CFLAGS)
							$(DRM_DEVEL_CFLAGS)
							
libgstsubrender_la_LIBADD = $(GST_LIBS) -lgstvideo-$(GST_MAJORMINOR) -lgstinterfaces-$(GST_MAJORMINOR) \
							$(GST_BASE_LIBS) \
							$(GSTPB_BASE_LIBS) \
							$(PANGO_LIBS) \
							$(TBM_LIBS) \
							$(DRI2_LIBS) \
							$(DRI2PROTO_LIBS) \
							$(X_LIBS) \
							$(XFIXES_LIBS) \
							$(XVIDEO_LIBS) \
							$(XSHM_LIBS) \
							$(X11_LIBS) \
							$(XV_LIBS) \
							$(DRM_LIBS) \
							$(DRM_DEVEL_LIBS)
							
libgstsubrender_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

# headers we need but don't want installed
noinst_HEADERS = gstsubrender.h 
