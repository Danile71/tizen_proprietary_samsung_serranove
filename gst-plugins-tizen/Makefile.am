
ACLOCAL_AMFLAGS = -I common/m4

aclocaldir = $(datadir)/aclocal

SUBDIRS = common

if USE_GST_TIZEN_ENCODEBIN
SUBDIRS += encodebin
endif

if USE_GST_TIZEN_EVASPIXMAPSINK
SUBDIRS += evaspixmapsink
endif

if USE_GST_TIZEN_XVIMAGESRC
SUBDIRS += xvimagesrc
endif

if USE_GST_TIZEN_DRMSRC
SUBDIRS += drmsrc
endif

if USE_GST_TIZEN_TOGGLE
SUBDIRS += toggle
endif

if USE_GST_TIZEN_PD_PUSHSRC
SUBDIRS += pdpushsrc
endif

if USE_GST_TIZEN_AUDIOTP
SUBDIRS += audiotp
endif

if USE_GST_TIZEN_AUDIOEQ
SUBDIRS += audioeq
endif

if USE_GST_TIZEN_WFDMANAGER
SUBDIRS += wfdmanager
endif

if USE_GST_TIZEN_WFDTSDEMUX
SUBDIRS += wfdtsdemux
endif

DIST_SUBDIRS = common

if USE_GST_TIZEN_ENCODEBIN
DIST_SUBDIRS += encodebin
endif

if USE_GST_TIZEN_XVIMAGESRC
DIST_SUBDIRS += xvimagesrc
endif

if USE_GST_TIZEN_DRMSRC
DIST_SUBDIRS += drmsrc
endif

if USE_GST_TIZEN_TOGGLE
DIST_SUBDIRS += toggle
endif

if USE_GST_TIZEN_WFDMANAGER
DIST_SUBDIRS += wfdmanager
endif

if USE_GST_TIZEN_WFDTSDEMUX
DIST_SUBDIRS += wfdtsdemux
endif


EXTRA_DIST = \
	gstreamer.spec gstreamer.spec.in \
	configure.ac autogen.sh depcomp \
	MAINTAINERS ABOUT-NLS RELEASE gstreamer.doap \
	$(win32)

check: check-exports

