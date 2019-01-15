if ENABLE_WFD_EXTENDED_FEATURES
lib_LTLIBRARIES = libwfdrtspstreamext.la

libwfdrtspstreamext_la_SOURCES = wfdrtspstream_ext.c

libwfdrtspstreamext_la_CFLAGS =  -I$(srcdir) $(GST_PLUGINS_BASE_CFLAGS) $(GST_CFLAGS)
libwfdrtspstreamext_la_LIBADD = $(GST_LIBS) $(GST_PLUGINS_BASE_LIBS) $(GST_BASE_LIBS) \
               -lgstrtp-@GST_MAJORMINOR@ -lgstrtsp-@GST_MAJORMINOR@

noinst_HEADERS = wfdrtspstream_ext.h
endif


plugin_LTLIBRARIES = libgstwfdrtspsrc.la

libgstwfdrtspsrc_la_SOURCES = gstwfdrtspsrc.c gstwfdrtspext.c

libgstwfdrtspsrc_la_CFLAGS = $(GST_PLUGINS_BASE_CFLAGS) $(GST_CFLAGS) $(VCONF_FLAGS) $(MM_WFD_COMMON_CFLAGS)

if ENABLE_WFD_EXTENDED_FEATURES
libgstwfdrtspsrc_la_CFLAGS += -DENABLE_WFD_EXTENDED_FEATURES
endif

libgstwfdrtspsrc_la_LIBADD = $(GST_LIBS) $(GST_PLUGINS_BASE_LIBS) $(GST_BASE_LIBS) \
               -lgstinterfaces-@GST_MAJORMINOR@ \
               $(MM_WFD_COMMON_LIBS) \
               -lgstrtp-@GST_MAJORMINOR@ -lgstrtsp-@GST_MAJORMINOR@ \
               $(WIN32_LIBS) \
               $(VCONF_LIBS)\
               $(WFDHALCONFIG_LIBS)

if ENABLE_WFD_EXTENDED_FEATURES
libgstwfdrtspsrc_la_LIBADD += $(srcdir)/libwfdrtspstreamext.la
endif

libgstwfdrtspsrc_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstwfdrtspsrc_la_LIBTOOLFLAGS = --tag=disable-static

noinst_HEADERS = gstwfdrtspsrc.h \
                 gstwfdrtspext.h
if ENABLE_WFD_EXTENDED_FEATURES
else
noinst_HEADERS += wfdrtspstream.h
endif

plugin_LTLIBRARIES += libgstwfdrtprequester.la

libgstwfdrtprequester_la_SOURCES = gstwfdrtprequester.c

libgstwfdrtprequester_la_CFLAGS = $(GST_PLUGINS_BASE_CFLAGS) $(GST_CFLAGS) $(VCONF_FLAGS) $(MM_WFD_COMMON_CFLAGS) $(WFDHALCONFIG_CFLAGS)
libgstwfdrtprequester_la_LIBADD = $(GST_LIBS) $(GST_PLUGINS_BASE_LIBS) $(GST_BASE_LIBS) \
               -lgstinterfaces-@GST_MAJORMINOR@ \
               $(MM_WFD_COMMON_LIBS) \
               -lgstrtp-@GST_MAJORMINOR@ -lgstrtsp-@GST_MAJORMINOR@ \
               $(WIN32_LIBS) \
               $(VCONF_LIBS)
libgstwfdrtprequester_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstwfdrtprequester_la_LIBTOOLFLAGS = --tag=disable-static

noinst_HEADERS = gstwfdrtprequester.h

