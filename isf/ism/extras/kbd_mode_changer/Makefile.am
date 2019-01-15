MAINTAINERCLEANFILES = Makefile.in
CLEANFILES           = *.bak

INCLUDES = -I$(top_builddir) \
           -I$(top_builddir)/ism/src \
           -I$(top_srcdir) \
           -I$(top_srcdir)/ism/src \
           -I$(top_srcdir)/ism/intl \
           -I$(top_srcdir)/ism/data \
           -I$(top_srcdir)/ism/utils \
           -I$(top_srcdir)/ism/extras/kbd_mode_changer/include \
           -I$(includedir) \
           -DSCIM_DATADIR=\"@SCIM_DATADIR@\" \
           -DSCIM_LOCALEDIR=\"@SCIM_LOCALEDIR@\" \
           -DSCIM_SYSCONFDIR=\"@SCIM_SYSCONFDIR@\" \
           -DSCIM_LIBEXECDIR=\"@SCIM_LIBEXECDIR@\" \
           -DSCIM_ICONDIR=\"@SCIM_ICONDIR@\" \
           -DSCIM_MODULE_PATH=\"@SCIM_MODULE_PATH@\" \
           -DSCIM_TEMPDIR=\"@SCIM_TEMPDIR@\"


noinst_HEADERS =

ISF_KBD_MODE_CHANGER = isf-kbd-mode-changer
appexecdir            = /usr/apps/org.tizen.isf-kbd-mode-changer/bin/
appexec_PROGRAMS      = $(ISF_KBD_MODE_CHANGER)

isf_kbd_mode_changer_SOURCES  = main.cpp

isf_kbd_mode_changer_CXXFLAGS = @EFL_CFLAGS@ \
                                @ELEMENTARY_CFLAGS@ \
                                @DLOG_CFLAGS@ \
                                @APPFW_CFLAGS@ \
                                -fPIE

isf_kbd_mode_changer_LDFLAGS  = @EFL_LIBS@ -rpath $(libdir)\
                                @ELEMENTARY_LIBS@ \
                                @DLOG_LIBS@ \
                                @APPFW_LIBS@ \
                                -pie
