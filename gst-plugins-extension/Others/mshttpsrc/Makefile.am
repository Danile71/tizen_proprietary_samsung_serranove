
plugin_LTLIBRARIES = libgstmshttpsrc.la

libgstmshttpsrc_la_SOURCES = gstmshttpsrc.c

libgstmshttpsrc_la_CFLAGS = $(GST_CFLAGS) $(GST_BASE_CFLAGS) $(SOUP_CFLAGS)
libgstmshttpsrc_la_LIBADD = $(GST_LIBS) $(GST_BASE_LIBS) $(SOUP_LIBS)
libgstmshttpsrc_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

# headers we need but don't want installed
noinst_HEADERS = gstmshttpsrc.h