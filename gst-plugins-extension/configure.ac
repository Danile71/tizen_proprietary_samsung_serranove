dnl Copyright (c) 2006 Software Laboratory, SAMSUNG Electronics, Inc.

dnl All rights reserved.
dnl
dnl Orignal by Jeechul jeon <heechul.jeon@samsung.com>

AC_INIT(extension, 1.0)
 
dnl versions of gstreamer and plugins-base
GST_MAJORMINOR=1.0
GST_REQUIRED=1.4.0
GSTPB_REQUIRED=1.4.0
EFL_REQUIRED=1.0.0

dnl fill in your package name and version here
dnl the fourth (nano) number should be 0 for a release, 1 for CVS,
dnl and 2... for a prerelease

dnl when going to/from release please set the nano correctly !
dnl releases only do Wall, cvs and prerelease does Werror too
AS_VERSION(gst-plugin, GST_PLUGIN_VERSION, 0, 10, 0, 1,
    GST_PLUGIN_CVS="no", GST_PLUGIN_CVS="yes")

dnl AM_MAINTAINER_MODE provides the option to enable maintainer mode
AM_MAINTAINER_MODE

#AM_INIT_AUTOMAKE($PACKAGE, $VERSION)
AM_INIT_AUTOMAKE

#AC_CONFIG_MACRO_DIR([m4])

dnl make aclocal work in maintainer mode
dnl AC_SUBST(ACLOCAL_AMFLAGS, "-I m4")

dnl Add parameters for aclocal
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4 -I common/m4")

AM_CONFIG_HEADER(config.h)

dnl check for tools
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

dnl decide on error flags
AS_COMPILER_FLAG(-Wall, GST_WALL="yes", GST_WALL="no")
                                                                                
if test "x$GST_WALL" = "xyes"; then
   GST_ERROR="$GST_ERROR -Wall"
                                                                                
#   if test "x$GST_PLUGIN_CVS" = "xyes"; then
#     AS_COMPILER_FLAG(-Werror,GST_ERROR="$GST_ERROR -Werror",GST_ERROR="$GST_ERROR")
#   fi
fi

dnl Check for pkgconfig first
AC_CHECK_PROG(HAVE_PKGCONFIG, pkg-config, yes, no)

dnl Give error and exit if we don't have pkgconfig
if test "x$HAVE_PKGCONFIG" = "xno"; then
  AC_MSG_ERROR(you need to have pkgconfig installed !)
fi

dnl Now we're ready to ask for gstreamer libs and cflags
dnl And we can also ask for the right version of gstreamer
AM_CONDITIONAL([IS_VODA_SDK], [test "x$DISTRO" = "xvodafone-sdk"])
AM_CONDITIONAL([ISPROTECTOR_VODA_SDK], [test "x$DISTRO" = "xvodafone-sdk" && test "x$MACHINE" = "xprotector"])
AM_CONDITIONAL([ISVOLANS_VODA_SDK], [test "x$DISTRO" = "xvodafone-sdk" && test "x$MACHINE" = "xvolans"])
AM_CONDITIONAL([ISPROTECTOR_TARGET], [test "x$ARCH" = "xarm" && test "x$MACHINE" = "xprotector"])
AM_CONDITIONAL([ISVOLANS_TARGET], [test "x$ARCH" = "xarm" && test "x$MACHINE" = "xvolans"])

PKG_CHECK_MODULES(GST, \
  gstreamer-$GST_MAJORMINOR >= $GST_REQUIRED,
  HAVE_GST=yes,HAVE_GST=no)

dnl Give error and exit if we don't have gstreamer
if test "x$HAVE_GST" = "xno"; then
  AC_MSG_ERROR(you need gstreamer development packages installed !)
fi

dnl append GST_ERROR cflags to GST_CFLAGS
GST_CFLAGS="$GST_CFLAGS $GST_ERROR"

dnl make GST_CFLAGS and GST_LIBS available
PKG_CHECK_MODULES(GST, gstreamer-0.10 >= 0.10)
AC_SUBST(GST_CFLAGS)
AC_SUBST(GST_LIBS)

dnl make GST_MAJORMINOR available in Makefile.am
AC_SUBST(GST_MAJORMINOR)

dnl If we need them, we can also use the base class libraries
PKG_CHECK_MODULES(GST_BASE, gstreamer-base-$GST_MAJORMINOR >= $GST_REQUIRED,
                  HAVE_GST_BASE=yes, HAVE_GST_BASE=no)

dnl Give a warning if we don't have gstreamer libs
dnl you can turn this into an error if you need them
if test "x$HAVE_GST_BASE" = "xno"; then
  AC_MSG_NOTICE(no GStreamer base class libraries found (gstreamer-base-$GST_MAJORMINOR))
fi

dnl make _CFLAGS and _LIBS available
AC_SUBST(GST_BASE_CFLAGS)
AC_SUBST(GST_BASE_LIBS)

dnl If we need them, we can also use the gstreamer-plugins-base libraries
PKG_CHECK_MODULES(GSTPB_BASE,
                  gstreamer-plugins-base-$GST_MAJORMINOR >= $GSTPB_REQUIRED,
                  HAVE_GSTPB_BASE=yes, HAVE_GSTPB_BASE=no)

dnl Give a warning if we don't have gstreamer libs
dnl you can turn this into an error if you need them
if test "x$HAVE_GSTPB_BASE" = "xno"; then
  AC_MSG_NOTICE(no GStreamer Plugins Base libraries found (gstreamer-plugins-base-$GST_MAJORMINOR))
fi

dnl make _CFLAGS and _LIBS available
AC_SUBST(GSTPB_BASE_CFLAGS)
AC_SUBST(GSTPB_BASE_LIBS)

dnl If we need them, we can also use the gstreamer-controller libraries
PKG_CHECK_MODULES(GSTCTRL,
                  gstreamer-controller-$GST_MAJORMINOR >= $GSTPB_REQUIRED,
                  HAVE_GSTCTRL=yes, HAVE_GSTCTRL=no)

dnl Give a warning if we don't have gstreamer-controller
dnl you can turn this into an error if you need them
if test "x$HAVE_GSTCTRL" = "xno"; then
  AC_MSG_NOTICE(no GStreamer Controller libraries found (gstreamer-controller-$GST_MAJORMINOR))
fi

dnl make _CFLAGS and _LIBS available
AC_SUBST(GSTCTRL_CFLAGS)
AC_SUBST(GSTCTRL_LIBS)

dnl set the plugindir where plugins should be installed
if test "x${prefix}" = "x$HOME"; then
  plugindir="$HOME/.gstreamer-$GST_MAJORMINOR/plugins"
else
  plugindir="\$(libdir)/gstreamer-$GST_MAJORMINOR"
fi
AC_SUBST(plugindir)

dnl set proper LDFLAGS for plugins
#GST_PLUGIN_LDFLAGS='-module -avoid-version -export-symbols-regex [_]*\(gst_\|Gst\|GST_\).*'
GST_PLUGIN_LDFLAGS='-module -avoid-version -export-symbols-regex [_]*\(gst_\|Gst\|GST_\|FIR_\|arkamys_malloc\|arkamys_free\).*'
#GST_PLUGIN_LDFLAGS='-module -avoid-version'
AC_SUBST(GST_PLUGIN_LDFLAGS)


PKG_CHECK_MODULES(GST_AUDIO, gstreamer-audio-$GST_MAJORMINOR >= $GST_REQUIRED )


dnl make _CFLAGS and _LIBS available
AC_SUBST(GST_AUDIO_CFLAGS)
AC_SUBST(GST_AUDIO_LIBS)

PKG_CHECK_MODULES(GST_VIDEO, gstreamer-video-$GST_MAJORMINOR >= $GST_REQUIRED)

dnl make _CFLAGS and _LIBS available
AC_SUBST(GST_VIDEO_CFLAGS)
AC_SUBST(GST_VIDEO_LIBS)

dnl use time analysis module
#PKG_CHECK_MODULES(MMTA, mm-ta)
#AC_SUBST(MMTA_CFLAGS)
#AC_SUBST(MMTA_LIBS)

dnl camsrcjpegenc
PKG_CHECK_MODULES(CAMSRCJPEGENC, camsrcjpegenc)
AC_SUBST(CAMSRCJPEGENC_CFLAGS)
AC_SUBST(CAMSRCJPEGENC_LIBS)

PKG_CHECK_MODULES(DRI2, libdri2)
AC_SUBST(DRI2_CFLAGS)
AC_SUBST(DRI2_LIBS)

PKG_CHECK_MODULES(DRM, libdrm)
AC_SUBST(DRM_CFLAGS)
AC_SUBST(DRM_LIBS)

PKG_CHECK_MODULES(DRM, libdrm-devel)
AC_SUBST(DRM_DEVEL_CFLAGS)
AC_SUBST(DRM_DEVEL_LIBS)

PKG_CHECK_MODULES(INIPARSER, iniparser)
AC_SUBST(INIPARSER_CFLAGS)
AC_SUBST(INIPARSER_LIBS)

dnl c2dconvert
PKG_CHECK_MODULES(XFIXES, xfixes)
AC_SUBST(XFIXES_CFLAGS)
AC_SUBST(XFIXES_LIBS)

PKG_CHECK_MODULES(XDAMAGE, xdamage)
AC_SUBST(XDAMAGE_CFLAGS)
AC_SUBST(XDAMAGE_LIBS)

PKG_CHECK_MODULES(DRI2PROTO, dri2proto)
AC_SUBST(DRI2PROTO_CFLAGS)
AC_SUBST(DRI2PROTO_LIBS)

PKG_CHECK_MODULES(TBM, libtbm)
AC_SUBST(TBM_CFLAGS)
AC_SUBST(TBM_LIBS)

dnl PKG_CHECK_MODULES(UDEVMGR, unified-dev-mgr)
dnl AC_SUBST(UDEVMGR_CFLAGS)
dnl AC_SUBST(UDEVMGR_LIBS)

dnl for vconf use ----------------------------------------------------------------------------
PKG_CHECK_MODULES(VCONF, vconf)
AC_SUBST(VCONF_CFLAGS)
AC_SUBST(VCONF_LIBS)


dnl use ext-imagereader --------------------------------------------------------------------------
AC_ARG_ENABLE(ext-imagereader, AC_HELP_STRING([--enable-ext-imagereader], [using imagereader]),
  [
    case "${enableval}" in
      yes) GST_EXT_USE_EXT_IMAGEREADER=yes ;;
      no)  GST_EXT_USE_EXT_IMAGEREADER=no ;;
      *)   AC_MSG_ERROR(bad value ${enableval} for --enable-ext-imagereader) ;;
    esac
  ],
  [GST_EXT_USE_EXT_IMAGEREADER=yes])
AM_CONDITIONAL(GST_EXT_USE_EXT_IMAGEREADER, test "x$GST_EXT_USE_EXT_IMAGEREADER" = "xyes")

dnl use ext-videomute --------------------------------------------------------------------------
AC_ARG_ENABLE(ext-videomute, AC_HELP_STRING([--enable-ext-videomute], [using videomute]),
  [
    case "${enableval}" in
      yes) GST_EXT_USE_EXT_VIDEOMUTE=yes ;;
      no)  GST_EXT_USE_EXT_VIDEOMUTE=no ;;
      *)   AC_MSG_ERROR(bad value ${enableval} for --enable-ext-videomute) ;;
    esac
  ],
  [GST_EXT_USE_EXT_VIDEOMUTE=yes])
AM_CONDITIONAL(GST_EXT_USE_EXT_VIDEOMUTE, test "x$GST_EXT_USE_EXT_VIDEOMUTE" = "xyes")

dnl use ext-submux --------------------------------------------------------------------------
AC_ARG_ENABLE(ext-submux, AC_HELP_STRING([--enable-ext-submux], [using submux]),
[
 case "${enableval}" in
         yes) GST_EXT_USE_EXT_SUBMUX=yes ;;
         no)  GST_EXT_USE_EXT_SUBMUX=no ;;
         *)   AC_MSG_ERROR(bad value ${enableval} for --enable-ext-submux) ;;
 esac
 ],
 [GST_EXT_USE_EXT_SUBMUX=yes])
AM_CONDITIONAL(GST_EXT_USE_EXT_SUBMUX, test "x$GST_EXT_USE_EXT_SUBMUX" = "xyes")

dnl use voicedatasrcbin--------------------------------------------------------------------
AC_ARG_ENABLE(ext-voicedatasrcbin, AC_HELP_STRING([--enable-ext-voicedatasrcbin], [using voicedatasrcbin]),
  [
    case "${enableval}" in
      yes) GST_EXT_USE_EXT_VOICEDATASRCBIN=yes ;;
      no)  GST_EXT_USE_EXT_VOICEDATASRCBIN=no ;;
      *)   AC_MSG_ERROR(bad value ${enableval} for --enable-ext-voicedatasrcbin) ;;
    esac 
  ],
  [GST_EXT_USE_EXT_VOICEDATASRCBIN=yes])
AM_CONDITIONAL(GST_EXT_USE_EXT_VOICEDATASRCBIN, test "x$GST_EXT_USE_EXT_VOICEDATASRCBIN" = "xyes")

dnl for i386 ------------------------------------------------------------------------------
AC_ARG_ENABLE(i386, AC_HELP_STRING([--enable-i386], [i386 build]),
	[
	 case "${enableval}" in
	  yes) IS_I386=yes ;;
	  no)  IS_I386=no ;;
	  *)   AC_MSG_ERROR(bad value ${enableval} for --enable-i386) ;;
	 esac
	],
	[IS_I386=no])
AM_CONDITIONAL([IS_I386], [test "x$IS_I386" = "xyes"])


dnl use ext-mutlisession-httpsrc --------------------------------------------------------------------------
AC_ARG_ENABLE(ext-multisession-httpsrc, AC_HELP_STRING([--enable-ext-multisession-httpsrc], [using multisession-httpsrc]),
[
 case "${enableval}" in
	yes) GST_EXT_USE_EXT_MULTISESSION_HTTPSRC=yes ;;
	no)  GST_EXT_USE_EXT_MULTISESSION_HTTPSRC=no ;;
	*)   AC_MSG_ERROR(bad value ${enableval} for --enable-ext-multisession-httpsrc) ;;
 esac
 ],
 [GST_EXT_USE_EXT_MULTISESSION_HTTPSRC=yes])
AM_CONDITIONAL(GST_EXT_USE_EXT_MULTISESSION_HTTPSRC, test "x$GST_EXT_USE_EXT_MULTISESSION_HTTPSRC" = "xyes")


dnl use ext-pip --------------------------------------------------------------------------
AC_ARG_ENABLE(ext-pip, AC_HELP_STRING([--enable-ext-pip], [using pip]),
[
 case "${enableval}" in
     yes) GST_EXT_USE_EXT_PIP=yes ;;
     no)  GST_EXT_USE_EXT_PIP=no ;;
     *)   AC_MSG_ERROR(bad value ${enableval} for --enable-ext-pip) ;;
 esac
 ],
 [GST_EXT_USE_EXT_PIP=yes])
AM_CONDITIONAL(GST_EXT_USE_EXT_PIP, test "x$GST_EXT_USE_EXT_PIP" = "xyes")
if [test "x$GST_EXT_USE_EXT_PIP" = "xyes"];
then
PKG_CHECK_MODULES(IMAGE_FILTER, image-filter)
AC_SUBST(IMAGE_FILTER_CFLAGS)
AC_SUBST(IMAGE_FILTER_LIBS)
PKG_CHECK_MODULES(MM_UTIL_JPEG, mmutil-jpeg)
AC_SUBST(MM_UTIL_JPEG_CFLAGS)
AC_SUBST(MM_UTIL_JPEG_LIBS)
fi

dnl enable wfd-extended-features  --------------------------------------------------------------------------
AC_ARG_ENABLE(wfd-extended-features, AC_HELP_STRING([--enable-wfd-extended-features], [samsung wfd spec]),
[
 case "${enableval}" in
         yes) ENABLE_WFD_EXTENDED_FEATURES=yes ;;
         no)  ENABLE_WFD_EXTENDED_FEATURES=no ;;
         *)   AC_MSG_ERROR(bad value ${enableval} for --enable-wfd-extended-features) ;;
 esac
 ],
 [ENABLE_WFD_EXTENDED_FEATURES=no])
AM_CONDITIONAL(ENABLE_WFD_EXTENDED_FEATURES, test "x$ENABLE_WFD_EXTENDED_FEATURES" = "xyes")

dnl use ext-wfdrtspsrc --------------------------------------------------------------------------
AC_ARG_ENABLE(ext-wfdrtspsrc, AC_HELP_STRING([--enable-ext-wfdrtspsrc], [using wfdrtspsrc]),
[
 case "${enableval}" in
	 yes) GST_EXT_USE_EXT_WFDRTSPSRC=yes ;;
	 no)  GST_EXT_USE_EXT_WFDRTSPSRC=no ;;
	 *)   AC_MSG_ERROR(bad value ${enableval} for --enable-ext-wfdrtspsrc) ;;
 esac
 ],
 [GST_EXT_USE_EXT_WFDRTSPSRC=yes])
AM_CONDITIONAL(GST_EXT_USE_EXT_WFDRTSPSRC, test "x$GST_EXT_USE_EXT_WFDRTSPSRC" = "xyes")
if [test "x$GST_EXT_USE_EXT_WFDRTSPSRC" = "xyes"];
then
PKG_CHECK_MODULES(MM_WFD_COMMON, mm-wfd-common)
AC_SUBST(MM_WFD_COMMON_CFLAGS)
AC_SUBST(MM_WFD_COMMON_LIBS)
fi

dnl use ext-evastextoverlay --------------------------------------------------------------------------
AC_ARG_ENABLE(ext-evastextoverlay, AC_HELP_STRING([--enable-ext-evastextoverlay], [using evastextoverlay]),
  [
    case "${enableval}" in
      yes) GST_EXT_USE_EXT_EVASTEXTOVERLAY=yes ;;
      no)  GST_EXT_USE_EXT_EVASTEXTOVERLAY=no ;;
      *)   AC_MSG_ERROR(bad value ${enableval} for --enable-ext-evastextoverlay) ;;
    esac
  ],
  [GST_EXT_USE_EXT_EVASTEXTOVERLAY=yes])
AM_CONDITIONAL(GST_EXT_USE_EXT_EVASTEXTOVERLAY, test "x$GST_EXT_USE_EXT_EVASTEXTOVERLAY" = "xyes")
if [test "x$GST_EXT_USE_EXT_EVASTEXTOVERLAY" = "xyes"];
then
PKG_CHECK_MODULES(EFL, [
  evas >= $EFL_REQUIRED
  ecore >= $EFL_REQUIRED
  ecore-x >= $EFL_REQUIRED
], [
  AC_SUBST(EFL_CFLAGS)
  AC_SUBST(EFL_LIBS)
], [
  AC_MSG_ERROR([
      You need to install or upgrade the EFL development
      packages on your system. On debian-based systems these are
      libevas-dev and libecore-dev.
      The minimum version required is $EFL_REQUIRED.
  ])
])
fi

dnl use ext-subrender --------------------------------------------------------------------------
AC_ARG_ENABLE(ext-subrender, AC_HELP_STRING([--enable-ext-subrender], [using subrender]),
[
 case "${enableval}" in
	 yes) GST_EXT_USE_EXT_SUBRENDER=yes ;;
	 no)  GST_EXT_USE_EXT_SUBRENDER=no ;;
	 *)   AC_MSG_ERROR(bad value ${enableval} for --enable-ext-subrender) ;;
 esac
 ],
 [GST_EXT_USE_EXT_SUBRENDER=yes])
AM_CONDITIONAL(GST_EXT_USE_EXT_SUBRENDER, test "x$GST_EXT_USE_EXT_SUBRENDER" = "xyes")

if [test "x$GST_EXT_USE_EXT_SUBRENDER" = "xyes"];
then
PKG_CHECK_MODULES(PANGO, pango pangocairo)
AC_SUBST(PANGO_CFLAGS)
AC_SUBST(PANGO_LIBS)

PKG_CHECK_MODULES(GST_VIDEO, gstreamer-video-0.10)
AC_SUBST(GST_VIDEO_CFLAGS)
AC_SUBST(GST_VIDEO_LIBS)

PKG_CHECK_MODULES(X11, x11)
AC_SUBST(X11_CFLAGS)
AC_SUBST(X11_LIBS)

PKG_CHECK_MODULES(XEXT, xext)
AC_SUBST(XEXT_CFLAGS)
AC_SUBST(XEXT_LIBS)

PKG_CHECK_MODULES(XV, xv)
AC_SUBST(XV_CFLAGS)
AC_SUBST(XV_LIBS)
fi

AG_GST_ARG_WITH_PACKAGE_NAME
AG_GST_ARG_WITH_PACKAGE_ORIGIN

dnl set license and copyright notice
GST_LICENSE="LGPL"
AC_DEFINE_UNQUOTED(GST_LICENSE, "$GST_LICENSE", [GStreamer license])
AC_SUBST(GST_LICENSE)

dnl *** subrender plug-ins ***
echo
AC_MSG_NOTICE([Checking libraries for subrender plugin])
echo
dnl *** X11 ***
translit(dnm, m, l) AM_CONDITIONAL(USE_X, true)
AG_GST_CHECK_FEATURE(X, [X libraries and plugins],
                  [subrender], [
  AC_PATH_XTRA
  ac_cflags_save="$CFLAGS"
  ac_cppflags_save="$CPPFLAGS"
  CFLAGS="$CFLAGS $X_CFLAGS"
  CPPFLAGS="$CPPFLAGS $X_CFLAGS"

  dnl now try to find the HEADER
  AC_CHECK_HEADER(X11/Xlib.h, HAVE_X="yes", HAVE_X="no")

  if test "x$HAVE_X" = "xno"
  then
    AC_MSG_NOTICE([cannot find X11 development files])
  else
    dnl this is much more than we want
    X_LIBS="$X_LIBS $X_PRE_LIBS $X_EXTRA_LIBS"
    dnl AC_PATH_XTRA only defines the path needed to find the X libs,
    dnl it does not add the libs; therefore we add them here
    X_LIBS="$X_LIBS -lX11"
    AC_SUBST(X_CFLAGS)
    AC_SUBST(X_LIBS)
  fi
  AC_SUBST(HAVE_X)
  CFLAGS="$ac_cflags_save"
  CPPFLAGS="$ac_cppflags_save"
])

dnl Check for Xv extension
translit(dnm, m, l) AM_CONDITIONAL(USE_XVIDEO, true)
AG_GST_CHECK_FEATURE(XVIDEO, [X11 XVideo extensions],
                  [subrender], [
AG_GST_CHECK_XV
])

dnl check for X Shm
translit(dnm, m, l) AM_CONDITIONAL(USE_XSHM, true)
AG_GST_CHECK_FEATURE(XSHM, [X Shared Memory extension], , [
  if test x$HAVE_X = xyes; then
    AC_CHECK_LIB(Xext, XShmAttach,
         HAVE_XSHM="yes", HAVE_XSHM="no",
         $X_LIBS)
    if test "x$HAVE_XSHM" = "xyes"; then
      XSHM_LIBS="-lXext"
    else
      dnl On AIX, it is in XextSam instead, but we still need -lXext
      AC_CHECK_LIB(XextSam, XShmAttach,
           HAVE_XSHM="yes", HAVE_XSHM="no",
           $X_LIBS)
      if test "x$HAVE_XSHM" = "xyes"; then
    XSHM_LIBS="-lXext -lXextSam"
      fi
    fi
  fi
], , [
  AC_SUBST(HAVE_XSHM)
  AC_SUBST(XSHM_LIBS)
])

AC_OUTPUT(
Makefile
common/Makefile
common/m4/Makefile
Others/Makefile
Others/imagereader/Makefile
Others/imagereader/src/Makefile
Others/videomute/Makefile
Others/videomute/src/Makefile
Others/splitter/Makefile
Others/splitter/src/Makefile
Others/voicedatasrcbin/Makefile
Others/voicedatasrcbin/src/Makefile
Others/socketsrc/Makefile
Others/socketsrc/src/Makefile
Others/mshttpsrc/Makefile
Others/wfdrtspsrc/Makefile
Others/wfdrtspsrc/src/Makefile
Others/pip/Makefile
Others/pip/src/Makefile
Others/subrender/Makefile
Others/subrender/src/Makefile
Others/submux/Makefile
Others/submux/src/Makefile
Others/evastextoverlay/Makefile
Others/evastextoverlay/src/Makefile
)
