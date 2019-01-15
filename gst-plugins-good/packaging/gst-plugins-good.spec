Name:           gst-plugins-good1.0
Summary:        GStreamer plugins from the "good" set
Version:        1.4.7
Release:        29
Group:          Applications/Multimedia
License:        LGPL
Source0:        %{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(gstreamer-1.0)
BuildRequires:  pkgconfig(gstreamer-plugins-base-1.0)
BuildRequires:  pkgconfig(libsoup-2.4)
BuildRequires:  libjpeg-turbo-devel
BuildRequires:  pkgconfig(libpng)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(libpulse) >= 1.0
#BuildRequires:	pkgconfig(pulseaudio)
BuildRequires:  gettext
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(xfixes)
BuildRequires:  pkgconfig(xdamage)
BuildRequires:  pkgconfig(xext)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(iniparser)
BuildRequires:  sec-product-features
#BuildRequires:  which
#BuildRequires:  pkgconfig(liboil-0.3)
#BuildRequires:  pkgconfig(drm-client)
#BuildRequires:  pkgconfig(drm-trusted)
#BuildRequires:  pkgconfig(libdrm)


%description
GStreamer is a streaming media framework, based on graphs of filters
which operate on media data.  Applications using this library can do
anything from real-time sound processing to playing videos, and just
about anything else media-related.  Its plugin-based architecture means
that new data types or processing capabilities can be added simply by
installing new plug-ins.
This package contains the GStreamer plugins from the "good" set, a set
of good-quality plug-ins under the LGPL license.

%package devel
Summary: Development files for the GStreamer media framework "good" plug-ins
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
Requires: gstreamer-plugins-base1.0-devel
Obsoletes: gstreamer-plugins-good1.0-devel < %{version}-%{release}
Provides: gstreamer-plugins-good1.0-devel = %{version}-%{release}

%description devel
GStreamer is a streaming media framework, based on graphs of elements which
operate on media data.

This package contains the development files for the plug-ins that
aren't tested well enough, or the code is not of good enough quality.

%prep
%setup -q


%build
%autogen --noconfigure

export CFLAGS+=" -DGST_EXT_V4L2_MODIFICATION\
 -DGST_EXT_WAVPARSE_MODIFICATION\
 -DMKV_DEMUX_MODIFICATION\
 -DGST_EXT_AVIDEMUX_MODIFICATION\
 -DGST_EXT_QTDEMUX_MODIFICATION\
 -DGST_EXT_AACPARSER_MODIFICATION\
 -DGST_EXT_BASEPARSER_MODIFICATION\
 -DGST_EXT_MP3PARSER_MODIFICATION\
 -DGST_EXT_RTSPSRC_MODIFICATION\
 -DGST_EXT_FLACPARSER_MODIFICATION\
 -DGST_EXT_WAVPARSER_MODIFICATION\
 -DGST_EXT_SOUP_MODIFICATION\
 -DGST_EXT_AMRPARSER_MODIFICATION\
 -DGST_EXT_ID3DEMUX_MODIFICATION"

%if ! 0%{?sec_product_feature_mmfw_audio_soundbooster}
export CFLAGS+=" -DUSE_PA_AUDIO_FILTER"
%endif

%configure --prefix=/usr\
%ifarch %{arm}
 --enable-divx-drm\
%endif
%if 0%{?tizen_build_binary_release_type_eng}
 --enable-pcmdump\
%endif
 --disable-nls\
 --disable-static\
 --with-html-dir=/tmp/dump\
 --disable-cairo\
 --disable-taglib\
 --disable-libcaca\
 --disable-flac\
 --disable-speex\
 --disable-shout2\
 --disable-shout2test\
 --disable-libdv\
 --disable-dv1394\
 --disable-aasink\
 --disable-jack\
 --disable-gdkpixbuf\
 --disable-wavpack\
 --disable-vpx

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp COPYING %{buildroot}/usr/share/license/%{name}
%make_install
rm -rf %{buildroot}/tmp/dump

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%manifest gst-plugins-good.manifest
%defattr(-,root,root,-)
%doc AUTHORS COPYING README REQUIREMENTS gst-plugins-good.doap
%{_datadir}/gstreamer-1.0/presets/GstIirEqualizer10Bands.prs
%{_datadir}/gstreamer-1.0/presets/GstIirEqualizer3Bands.prs
#%{_datadir}/gstreamer-1.0/presets/GstVP8Enc.prs

# non-core plugins without external dependencies
%{_libdir}/gstreamer-1.0/libgstalaw.so
%{_libdir}/gstreamer-1.0/libgstalpha.so
%{_libdir}/gstreamer-1.0/libgstautodetect.so
%{_libdir}/gstreamer-1.0/libgstavi.so
%{_libdir}/gstreamer-1.0/libgsteffectv.so
%{_libdir}/gstreamer-1.0/libgstgoom.so
%{_libdir}/gstreamer-1.0/libgstlevel.so
%{_libdir}/gstreamer-1.0/libgstmulaw.so
%{_libdir}/gstreamer-1.0/libgstisomp4.so
%{_libdir}/gstreamer-1.0/libgstrtp.so
%{_libdir}/gstreamer-1.0/libgstrtpmanager.so
%{_libdir}/gstreamer-1.0/libgstrtsp.so
%{_libdir}/gstreamer-1.0/libgstsmpte.so
%{_libdir}/gstreamer-1.0/libgstudp.so
%{_libdir}/gstreamer-1.0/libgstvideobox.so
%{_libdir}/gstreamer-1.0/libgstwavenc.so
%{_libdir}/gstreamer-1.0/libgstwavparse.so
%{_libdir}/gstreamer-1.0/libgstauparse.so
%{_libdir}/gstreamer-1.0/libgstdebug.so
%{_libdir}/gstreamer-1.0/libgstnavigationtest.so
%{_libdir}/gstreamer-1.0/libgstalphacolor.so
#%{_libdir}/gstreamer-1.0/libgstcairo.so
%{_libdir}/gstreamer-1.0/libgstflxdec.so
%{_libdir}/gstreamer-1.0/libgstmatroska.so
%{_libdir}/gstreamer-1.0/libgstvideomixer.so
%{_libdir}/gstreamer-1.0/libgstcutter.so
%{_libdir}/gstreamer-1.0/libgstmultipart.so
%{_libdir}/gstreamer-1.0/libgstid3demux.so
#%{_libdir}/gstreamer-1.0/libgstgdkpixbuf.so
%{_libdir}/gstreamer-1.0/libgstapetag.so
# %{_libdir}/gstreamer-1.0/libgstannodex.so
%{_libdir}/gstreamer-1.0/libgstvideocrop.so
%{_libdir}/gstreamer-1.0/libgsticydemux.so
#%{_libdir}/gstreamer-1.0/libgsttaglib.so
%{_libdir}/gstreamer-1.0/libgstximagesrc.so
%{_libdir}/gstreamer-1.0/libgstaudiofx.so
%{_libdir}/gstreamer-1.0/libgstequalizer.so
%{_libdir}/gstreamer-1.0/libgstmultifile.so
%{_libdir}/gstreamer-1.0/libgstspectrum.so
%{_libdir}/gstreamer-1.0/libgstgoom2k1.so
%{_libdir}/gstreamer-1.0/libgstinterleave.so
%{_libdir}/gstreamer-1.0/libgstreplaygain.so
%{_libdir}/gstreamer-1.0/libgstdeinterlace.so
%{_libdir}/gstreamer-1.0/libgstflv.so
%{_libdir}/gstreamer-1.0/libgsty4menc.so
%{_libdir}/gstreamer-1.0/libgstoss4audio.so
%{_libdir}/gstreamer-1.0/libgstimagefreeze.so
%{_libdir}/gstreamer-1.0/libgstshapewipe.so
%{_libdir}/gstreamer-1.0/libgstvideofilter.so
%{_libdir}/gstreamer-1.0/libgstaudioparsers.so
%{_libdir}/gstreamer-1.0/libgstdtmf.so

# sys plugins
#%{_libdir}/gstreamer-1.0/libgstvideo4linux2.so

# gstreamer-plugins with external dependencies but in the main package
#%{_libdir}/gstreamer-1.0/libgstcacasink.so
#%{_libdir}/gstreamer-1.0/libgstflac.so
#%{_libdir}/gstreamer-1.0/libgstjack.so
%{_libdir}/gstreamer-1.0/libgstjpeg.so
%{_libdir}/gstreamer-1.0/libgstpng.so
%{_libdir}/gstreamer-1.0/libgstossaudio.so
#%{_libdir}/gstreamer-1.0/libgstspeex.so
#%{_libdir}/gstreamer-1.0/libgstshout2.so
#%{_libdir}/gstreamer-1.0/libgstaasink.so
#%{_libdir}/gstreamer-1.0/libgstdv.so
#%{_libdir}/gstreamer-1.0/libgst1394.so
#%{_libdir}/gstreamer-1.0/libgstwavpack.so
%{_libdir}/gstreamer-1.0/libgstsouphttpsrc.so
%{_libdir}/gstreamer-1.0/libgstpulse.so
#%{_libdir}/gstreamer-1.0/libgstvpx.so

#%{_libdir}/libgsttuner-1.0.so*

/usr/share/license/%{name}

#%files devel
#%manifest gst-plugins-good-devel.manifest
#%defattr(-,root,root,-)
#%{_libdir}/libgsttuner-1.0.so*
#%{_includedir}/gstreamer-1.0/gst/tuner

#%{_libdir}/pkgconfig/gstreamer-plugins-good-1.0.pc
#%{_libdir}/pkgconfig/gstreamer-tuner-1.0.pc
