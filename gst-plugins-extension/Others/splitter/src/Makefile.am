# plugindir is set in configure

##############################################################################
# change libgstjpegdecoder.la to something more suitable, e.g. libmysomething.la  #
##############################################################################
plugin_LTLIBRARIES = libgstaudiosplitter.la

##############################################################################
# for the next set of variables, rename the prefix if you renamed the .la,   #
#  e.g. libgstjpegdecoder_la_SOURCES => libmysomething_la_SOURCES                 #
#       libgstjpegdecoder_la_CFLAGS  => libmysomething_la_CFLAGS                  #
#       libgstjpegdecoder_la_LIBADD  => libmysomething_la_LIBADD                  #
#       libgstjpegdecoder_la_LDFLAGS => libmysomething_la_LDFLAGS                 #
##############################################################################

# sources used to compile this plug-in
libgstaudiosplitter_la_SOURCES = gstaudiosplitter.c

# flags used to compile this plugin
# add other _CFLAGS and _LIBS as needed
libgstaudiosplitter_la_CFLAGS = $(GST_BASE_CFLAGS) $(GST_CFLAGS)
libgstaudiosplitter_la_LIBADD = $(GST_BASE_LIBS) $(GST_LIBS)
libgstaudiosplitter_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

# headers we need but don't want installed
noinst_HEADERS = gstaudiosplitter.h
