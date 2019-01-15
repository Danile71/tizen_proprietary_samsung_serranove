# plugindir is set in configure

##############################################################################
# change libgstjpegdecoder.la to something more suitable, e.g. libmysomething.la  #
##############################################################################
plugin_LTLIBRARIES = libgstvoicedatasrcbin.la

##############################################################################
# for the next set of variables, rename the prefix if you renamed the .la,   #
#  e.g. libgstjpegdecoder_la_SOURCES => libmysomething_la_SOURCES                 #
#       libgstjpegdecoder_la_CFLAGS  => libmysomething_la_CFLAGS                  #
#       libgstjpegdecoder_la_LIBADD  => libmysomething_la_LIBADD                  #
#       libgstjpegdecoder_la_LDFLAGS => libmysomething_la_LDFLAGS                 #
##############################################################################

# sources used to compile this plug-in
libgstvoicedatasrcbin_la_SOURCES = gstvoicedatasrcbin.c

# flags used to compile this plugin
# add other _CFLAGS and _LIBS as needed
libgstvoicedatasrcbin_la_CFLAGS = $(GST_CFLAGS)
libgstvoicedatasrcbin_la_LIBADD = $(GST_LIBS)
libgstvoicedatasrcbin_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

# headers we need but don't want installed
noinst_HEADERS = gstvoicedatasrcbin.h
