Name:       gst-plugins-bad1.0
Summary:    GStreamer plugins from the "bad" set
Version:    1.4.5
Release:    11
Group:      Applications/Multimedia
License:    LGPLv2+
Source0:    %{name}-%{version}.tar.gz
Patch0:     gst-plugins-bad-disable-gtk-doc.patch
BuildRequires:  gettext
BuildRequires:  python
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gstreamer-1.0)
BuildRequires:  pkgconfig(gstreamer-plugins-base-1.0)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(nettle)
#BuildRequires:  pkgconfig(librtmp)

#BuildRequires:  gst-plugins-base-devel
BuildRequires:  pkgconfig(libcrypto)

%description
GStreamer is a streaming media framework, based on graphs of filters
which operate on media data.  Applications using this library can do
anything from real-time sound processing to playing videos, and just
about anything else media-related.  Its plugin-based architecture means
that new data types or processing capabilities can be added simply by
installing new plug-ins.
.
GStreamer Bad Plug-ins is a set of plug-ins that aren't up to par compared
to the rest. They might be close to being good quality, but they're missing
something - be it a good code review, some documentation, a set of tests, a
real live maintainer, or some actual wide use.


%package devel
Summary: Development files for the GStreamer media framework "bad" plug-ins
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
Requires: gstreamer-plugins-base1.0-devel
Obsoletes: gstreamer-plugins-bad1.0-devel < %{version}-%{release}
Provides: gstreamer-plugins-bad1.0-devel = %{version}-%{release}

%description devel
GStreamer is a streaming media framework, based on graphs of elements which
operate on media data.

This package contains the development files for the plug-ins that
aren't tested well enough, or the code is not of good enough quality.


%prep
%setup -q
%patch0 -p1


%build
./autogen.sh

export CFLAGS+=" -Wall -g -fPIC\
 -DGST_EXT_HLS_MODIFICATION\
 -DGST_H263PARSE_MODIFICATION\
 -DGST_H264PARSE_MODIFICATION\
 -DGST_ADAPTIVE_MODIFICATION\
 -DGST_TSDEMUX_MODIFICATION\
 -DGST_DASHDEMUX_MODIFICATION\
 -DGST_BUG_PATCH\
 -DGST_EXT_AVOID_PAD_SWITCHING"

%configure  --prefix=%{_prefix}\
 --disable-static\
 --disable-nls\
 --with-html-dir=/tmp/dump\
 --disable-aiff\
 --disable-amrparse\
 --disable-asfmux\
 --disable-bayer\
 --disable-cdxaparse\
 --disable-dccp\
 --disable-dvdspu\
 --disable-festival\
 --disable-freeze\
 --disable-frei0r\
 --disable-hdvparse\
 --disable-librfb\
 --disable-modplug\
 --disable-mpegpsmux\
 --disable-mve\
 --disable-mxf\
 --disable-nsf\
 --disable-nuvdemux\
 --disable-pcapparse\
 --disable-pnm\
 --disable-qtmux\
 --disable-real\
 --disable-scaletempo\
 --disable-shapewipe\
 --disable-siren\
 --disable-speed\
 --disable-subenc\
 --disable-stereo\
 --disable-tta\
 --disable-videomeasure\
 --disable-videosignal\
 --disable-vmnc\
 --disable-directsound\
 --disable-directdraw\
 --disable-osx_video\
 --disable-vcd\
 --disable-assrender\
 --disable-amrwb\
 --disable-apexsink\
 --disable-bz2\
 --disable-cdaudio\
 --disable-celt\
 --disable-cog\
 --disable-dc1394\
 --disable-directfb\
 --disable-dirac\
 --disable-dts\
 --disable-divx\
 --disable-dvdnav\
 --disable-faac\
 --disable-faad\
 --disable-fbdev\
 --disable-gsm\
 --disable-ivorbis\
 --disable-jack\
 --disable-jp2k\
 --disable-kate\
 --disable-ladspa\
 --disable-lv2\
 --disable-libmms\
 --disable-modplug\
 --disable-midi\
 --disable-mimic\
 --disable-mpeg2enc\
 --disable-mplex\
 --disable-musepack\
 --disable-musicbrainz\
 --disable-mythtv\
 --disable-nas\
 --disable-neon\
 --disable-ofa\
 --disable-timidity\
 --disable-wildmidi\
 --disable-sdl\
 --disable-sdltest\
 --disable-sndfile\
 --disable-soundtouch\
 --disable-spc\
 --disable-gme\
 --disable-swfdec\
 --disable-theoradec\
 --disable-xvid\
 --disable-oss4\
 --disable-wininet\
 --disable-acm\
 --disable-vdpau\
 --disable-schro\
 --disable-vp8\
 --disable-zbar\
 --disable-dataurisrc\
 --disable-shm\
 --disable-coloreffects\
 --disable-colorspace\
 --disable-videomaxrate\
 --disable-jp2kdecimator\
 --disable-interlace\
 --disable-gaudieffects\
 --disable-y4m\
 --disable-adpcmenc\
 --disable-segmentclip\
 --disable-geometrictransform\
 --disable-inter\
 --disable-dvbsuboverlay\
 --disable-dvb\
 --disable-ivfparse\
 --disable-gsettings\
 --disable-opus\
 --disable-openal\
 --disable-snapdsp\
 --disable-teletextdec\
 --disable-audiovisualizers\
 --disable-faceoverlay\
 --disable-freeverb\
 --disable-removesilence\
 --disable-smooth\
 --disable-avc\
 --disable-d3dvideosink\
 --disable-pvr2d\
 --enable-wfd-tsdemux


make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp COPYING.LIB %{buildroot}/usr/share/license/%{name}
%make_install
rm -rf %{buildroot}/tmp/dump


%files
%manifest gst-plugins-bad.manifest
%defattr(-,root,root,-)
#%{_libdir}/libgstbasevideo-0.10.so.*
#%{_libdir}/libgstcodecparsers-0.10.so.*
#%{_libdir}/libgstbasecamerabinsrc-0.10.so.*
#%{_libdir}/libgstphotography-0.10.so.*
#%{_libdir}/libgstsignalprocessor-0.10.so.*
#%{_libdir}/gstreamer-0.10/libgst*.so

# Plugins without external dependencies
# %{_libdir}/gstreamer-1.0/libgstasfmux.so
#%{_libdir}/gstreamer-1.0/libgstdvdspu.so
# %{_libdir}/gstreamer-1.0/libgstmpegpsmux.so
#%{_libdir}/gstreamer-1.0/libgstmpegtsmux.so
#%ifarch %{ix86} x86_64
#%{_libdir}/gstreamer-1.0/libgstreal.so
#%endif
#%{_libdir}/gstreamer-1.0/libgstsiren.so

# Plugins with external dependencies
#%{_libdir}/gstreamer-1.0/libgstdtsdec.so
#%{_libdir}/gstreamer-1.0/libgstfaad.so
#%{_libdir}/gstreamer-1.0/libgstmms.so
#%{_libdir}/gstreamer-1.0/libgstmimic.so
#%{_libdir}/gstreamer-1.0/libgstmpeg2enc.so
#%{_libdir}/gstreamer-1.0/libgstmplex.so
#%{_libdir}/gstreamer-1.0/libgstxvid.so
#%{_libdir}/gstreamer-1.0/libgstfaac.so
#%{_libdir}/gstreamer-1.0/libgstmfc.so
#%{_libdir}/gstreamer-1.0/libgstvbidec.so
#%{_libdir}/gstreamer-1.0/libgstmpegpsmux.so
%{_libdir}/gstreamer-1.0/libgstaudiomixer.so
%{_libdir}/gstreamer-1.0/libgstcompositor.so

%doc AUTHORS COPYING README REQUIREMENTS
#%{_datadir}/gstreamer-1.0
#??? %{_libdir}/libgstbasevideo-1.0.so.*
%{_libdir}/libgstphotography-1.0.so.*
#??? %{_libdir}/libgstsignalprocessor-1.0.so.*
%{_libdir}/libgstcodecparsers-1.0.so.*
%{_libdir}/libgstinsertbin-1.0.so.*
%{_libdir}/libgstbadbase-1.0.so.*
%{_libdir}/libgstbadvideo-1.0.so.*
# Plugins without external dependencies
%{_libdir}/gstreamer-1.0/libgstadpcmdec.so
#%{_libdir}/gstreamer-1.0/libgstadpcmenc.so
# %{_libdir}/gstreamer-1.0/libgstaiff.so
%{_libdir}/gstreamer-1.0/libgstautoconvert.so
#%{_libdir}/gstreamer-1.0/libgstccdecbad.so
#%{_libdir}/gstreamer-1.0/libgstbayer.so
# %{_libdir}/gstreamer-1.0/libgstcamerabin.so
# %{_libdir}/gstreamer-1.0/libgstcdxaparse.so
# %{_libdir}/gstreamer-1.0/libgstcoloreffects.so
#%{_libdir}/gstreamer-1.0/libgstdataurisrc.so
# %{_libdir}/gstreamer-1.0/libgstdccp.so
# %{_libdir}/gstreamer-1.0/libgstfestival.so
# %{_libdir}/gstreamer-1.0/libgstfrei0r.so
# %{_libdir}/gstreamer-1.0/libgstgaudieffects.so
# %{_libdir}/gstreamer-1.0/libgstgeometrictransform.so
# %{_libdir}/gstreamer-1.0/libgstgsettingselements.so
# %{_libdir}/gstreamer-1.0/libgsthdvparse.so
# %{_libdir}/gstreamer-1.0/libgstid3tag.so
# %{_libdir}/gstreamer-1.0/libgstivfparse.so
%{_libdir}/gstreamer-1.0/libgstjpegformat.so
%{_libdir}/gstreamer-1.0/libgstliveadder.so
# %{_libdir}/gstreamer-1.0/libgstmpegdemux.so
# %{_libdir}/gstreamer-1.0/libgstmve.so
# %{_libdir}/gstreamer-1.0/libgstmxf.so
# %{_libdir}/gstreamer-1.0/libgstnsf.so
# %{_libdir}/gstreamer-1.0/libgstnuvdemux.so
#%{_libdir}/gstreamer-1.0/libgstpcapparse.so
#%{_libdir}/gstreamer-1.0/libgstpnm.so
# %{_libdir}/gstreamer-1.0/libgstrfbsrc.so
#%{_libdir}/gstreamer-1.0/libgstsegmentclip.so
%{_libdir}/gstreamer-1.0/libgstrawparse.so
#%{_libdir}/gstreamer-1.0/libgstshm.so
%{_libdir}/gstreamer-1.0/libgstsdpelem.so
#%{_libdir}/gstreamer-1.0/libgstsmooth.so
#%{_libdir}/gstreamer-1.0/libgstspeed.so
# %{_libdir}/gstreamer-1.0/libgststereo.so
# %{_libdir}/gstreamer-1.0/libgstsubenc.so
# %{_libdir}/gstreamer-1.0/libgsttta.so
# %{_libdir}/gstreamer-1.0/libgstvideosignal.so
# %{_libdir}/gstreamer-1.0/libgstvideomeasure.so
# %{_libdir}/gstreamer-1.0/libgstvmnc.so
%{_libdir}/gstreamer-1.0/libgstcamerabin2.so
# %{_libdir}/gstreamer-1.0/libgstcolorspace.so
#%{_libdir}/gstreamer-1.0/libgstcurl.so
%{_libdir}/gstreamer-1.0/libgstgdp.so
%{_libdir}/gstreamer-1.0/libgstaccurip.so
%{_libdir}/gstreamer-1.0/libgstfieldanalysis.so
#%{_libdir}/gstreamer-1.0/libgstmxf.so
#%{_libdir}/gstreamer-1.0/libgstsubenc.so
%{_libdir}/libgstbasecamerabinsrc-1.0.so.*

# %{_libdir}/gstreamer-1.0/libgstdecklink.so
#%{_libdir}/gstreamer-1.0/libgstdvbsuboverlay.so
# %{_libdir}/gstreamer-1.0/libgstfieldanalysis.so
#%{_libdir}/gstreamer-1.0/libgstfragmented.so
# %{_libdir}/gstreamer-1.0/libgstinterlace.so
# %{_libdir}/gstreamer-1.0/libgstjp2kdecimator.so
# %{_libdir}/gstreamer-1.0/libgstlinsys.so
# %{_libdir}/gstreamer-1.0/libgstmpegtsdemux.so
# %{_libdir}/gstreamer-1.0/libgstopencv.so
# %{_libdir}/gstreamer-1.0/libgstpatchdetect.so
# %{_libdir}/gstreamer-1.0/libgstsdi.so
# %{_libdir}/gstreamer-1.0/libgstvideofiltersbad.so
%{_libdir}/gstreamer-1.0/libgstvideoparsersbad.so
#%{_libdir}/gstreamer-1.0/libgsty4mdec.so
# %{_libdir}/gstreamer-1.0/libgstopenal.so
# %{_libdir}/libgstbasecamerabinsrc-1.0.so.24
# %{_libdir}/libgstbasecamerabinsrc-1.0.so.24.0.0
#%{_libdir}/gstreamer-1.0/libgstaudiovisualizers.so
# %{_libdir}/gstreamer-1.0/libgstfaceoverlay.so
#%{_libdir}/gstreamer-1.0/libgstinter.so
#%{_libdir}/gstreamer-1.0/libgstremovesilence.so
#%{_libdir}/gstreamer-1.0/libgstasfmux.so
#%{_libdir}/gstreamer-1.0/libgstcoloreffects.so
#%{_libdir}/gstreamer-1.0/libgstdvb.so
#%{_libdir}/gstreamer-1.0/libgstfestival.so
#%{_libdir}/gstreamer-1.0/libgstgaudieffects.so
#%{_libdir}/gstreamer-1.0/libgstgeometrictransform.so
%{_libdir}/gstreamer-1.0/libgstid3tag.so
#%{_libdir}/gstreamer-1.0/libgstinterlace.so
#%{_libdir}/gstreamer-1.0/libgstmimic.so
%{_libdir}/gstreamer-1.0/libgstmpegpsdemux.so
%{_libdir}/gstreamer-1.0/libgstmpegtsdemux.so
%{_libdir}/gstreamer-1.0/libgstmpegtsmux.so
#%{_libdir}/gstreamer-1.0/libgstsoundtouch.so
#%{_libdir}/libgstbasecamerabinsrc-1.0.so.0
# %{_libdir}/libgstbasecamerabinsrc-1.0.so.0.0.0

# System (Linux) specific plugins
# %{_libdir}/gstreamer-1.0/libgstdvb.so
# %{_libdir}/gstreamer-1.0/libgstvcdsrc.so

# Plugins with external dependencies
# %{_libdir}/gstreamer-1.0/libgstapexsink.so
#%{_libdir}/gstreamer-1.0/libgstassrender.so
#%{_libdir}/gstreamer-1.0/libgstbz2.so
# %{_libdir}/gstreamer-1.0/libgstcdaudio.so
#%{_libdir}/gstreamer-1.0/libgstcelt.so
#%{_libdir}/gstreamer-1.0/libgstopus.so
#%ifnarch s390 s390x
#%{_libdir}/gstreamer-1.0/libgstdc1394.so
#%endif
# %{_libdir}/gstreamer-1.0/libgstgsm.so
#%{_libdir}/gstreamer-1.0/libgstjp2k.so
#%{_libdir}/gstreamer-1.0/libgstkate.so
#%{_libdir}/gstreamer-1.0/libgstladspa.so
#%{_libdir}/gstreamer-1.0/libgstlv2.so
#%{_libdir}/gstreamer-1.0/libgstmodplug.so
#%{_libdir}/gstreamer-1.0/libgstmusepack.so
#%{_libdir}/gstreamer-1.0/libgstofa.so
# %{_libdir}/gstreamer-1.0/libgstrsvg.so
#%{_libdir}/gstreamer-1.0/libgstschro.so
# %{_libdir}/gstreamer-1.0/libgstsndfile.so
#%{_libdir}/gstreamer-1.0/libgstfrei0r.so
#%{_libdir}/gstreamer-1.0/libgstopencv.so
#%{_libdir}/gstreamer-1.0/libgstvoamrwbenc.so
#%{_datadir}/gstreamer-1.0/presets/*.prs

#debugging plugin
%{_libdir}/gstreamer-1.0/libgstdebugutilsbad.so

#data for plugins
# %{_datadir}/glib-2.0/schemas/org.freedesktop.gstreamer-0.11.default-elements.gschema.xml

%{_libdir}/gstreamer-1.0/libgstaudiofxbad.so
%{_libdir}/gstreamer-1.0/libgstdecklink.so
%{_libdir}/gstreamer-1.0/libgstivtc.so
#%{_libdir}/gstreamer-1.0/libgstmidi.so
%{_libdir}/gstreamer-1.0/libgstvideofiltersbad.so
%{_libdir}/gstreamer-1.0/libgstyadif.so
%{_libdir}/libgstmpegts-1.0.so*
%{_libdir}/libgsturidownloader-1.0.so*
%{_libdir}/libgstadaptivedemux-1.0.so*

#streaming plugins
%{_libdir}/gstreamer-1.0/libgstfragmented.so
%{_libdir}/gstreamer-1.0/libgstsmoothstreaming.so
%{_libdir}/gstreamer-1.0/libgstdashdemux.so
#%{_libdir}/gstreamer-1.0/libgstrtmp.so

/usr/share/license/%{name}


%files devel
%manifest gst-plugins-bad-devel.manifest
%defattr(-,root,root,-)
#%{_libdir}/libgst*.so
#%{_includedir}/gstreamer-0.10/gst/*
#%{_libdir}/pkgconfig/gstreamer-*.pc

#??? %{_libdir}/libgstbasevideo-1.0.so
%{_libdir}/libgstphotography-1.0.so
#??? %{_libdir}/libgstsignalprocessor-1.0.so
%{_libdir}/libgstinsertbin-1.0.so
%{_libdir}/libgstcodecparsers-1.0.so
%{_libdir}/libgstbadbase-1.0.so
%{_libdir}/libgstbadvideo-1.0.so
%{_libdir}/libgstbasecamerabinsrc-1.0.so
%{_includedir}/gstreamer-1.0/gst/interfaces/photography*
%{_includedir}/gstreamer-1.0/gst/codecparsers
%{_includedir}/gstreamer-1.0/gst/insertbin
#%{_includedir}/gstreamer-1.0/gst/signalprocessor
#%{_includedir}/gstreamer-1.0/gst/video
%{_includedir}/gstreamer-1.0/gst/mpegts
%{_includedir}/gstreamer-1.0/gst/uridownloader
%{_includedir}/gstreamer-1.0/gst/basecamerabinsrc/gstbasecamerasrc.h
%{_includedir}/gstreamer-1.0/gst/basecamerabinsrc/gstcamerabin-enum.h
%{_includedir}/gstreamer-1.0/gst/basecamerabinsrc/gstcamerabinpreview.h
%{_libdir}/pkgconfig/*.pc

