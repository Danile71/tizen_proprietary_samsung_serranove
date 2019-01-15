Name:              gst-plugins-base1.0
Summary:           GStreamer streaming media framework base plug-ins
Version:           1.4.20
Release:           3
Group:             Applications/Multimedia
License:           LGPL
Source0:           %{name}-%{version}.tar.gz
Requires(post):    /sbin/ldconfig
Requires(postun):  /sbin/ldconfig
BuildRequires:	pkgconfig(mm-common)
BuildRequires:  pkgconfig(ogg)
BuildRequires:  pkgconfig(theora)
BuildRequires:  pkgconfig(vorbis)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(alsa)
BuildRequires:  pkgconfig(gstreamer-1.0)
BuildRequires:  pkgconfig(xv)
BuildRequires:  pkgconfig(xfixes)
BuildRequires:  pkgconfig(dri2proto)
BuildRequires:  pkgconfig(libdri2)
BuildRequires:  pkgconfig(libtbm)
BuildRequires:  pkgconfig(libdrm)
BuildRequires:  intltool

%description
GStreamer is a streaming media framework, based on graphs of filters which
operate on media data. Applications using this library can do anything
from real-time sound processing to playing videos, and just about anything
else media-related.  Its plugin-based architecture means that new data
types or processing capabilities can be added simply by installing new
plug-ins.


%package devel
Summary:    GStreamer Plugin Library Headers
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
Obsoletes: gstreamer-plugins-base1.0-devel < %{version}-%{release}
Provides: gstreamer-plugins-base1.0-devel = %{version}-%{release}

%description devel
GStreamer Plugins Base library development and header files.


%prep
%setup -q

%build
%autogen --noconfigure

export CFLAGS+=" -Wall -g -fPIC\
 -DGST_EXT_AUDIODECODER_MODIFICATION\
 -DGST_EXT_XV_ENHANCEMENT\
 -DGST_EXT_LINK_FIMCCONVERT\
 -DGST_EXT_TYPEFIND_ENHANCEMENT\
 -DGST_EXT_MIME_TYPES\
 -DSUBPARSE_MODIFICATION\
 -DGST_EXT_VIDEODECODER\
 -DGST_UPSTREAM\
 -DUSE_FLUSH_BUFFER\
 -DGST_EXT_VIDEOENCODER\
 -DGST_EXT_RIFFMEDIA_MODIFICATION\
 -DGST_EXT_ID3_MODIFICATION"

%configure --prefix=/usr\
 --disable-static\
 --disable-nls\
 --with-html-dir=/tmp/dump\
 --disable-examples\
 --disable-audiorate\
 --disable-gdp\
 --disable-cdparanoia\
 --disable-gnome_vfs\
 --disable-libvisual\
 --disable-freetypetest\
 --disable-rpath\
 --disable-valgrind\
 --disable-gcov\
 --disable-gtk-doc\
 --disable-debug\
 --disable-pango\
 --with-audioresample-format=int

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp COPYING.LIB %{buildroot}/usr/share/license/%{name}
%make_install
rm -rf %{buildroot}/tmp/dump

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%manifest gst-plugins-base.manifest
%defattr(-,root,root,-)
%doc AUTHORS COPYING README REQUIREMENTS gst-plugins-base.doap

# helper programs
%{_bindir}/gst-discoverer-1.0
%{_mandir}/man1/gst-discoverer-1.0*
%{_bindir}/gst-play-1.0
%{_mandir}/man1/gst-play-1.0*
%{_bindir}/gst-device-monitor-1.0
%{_mandir}/man1/gst-device-monitor-1.0*

# libraries
%{_libdir}/libgstaudio-1.0.so.*
%{_libdir}/libgstpbutils-1.0.so*
%{_libdir}/libgstriff-1.0.so.*
%{_libdir}/libgstrtp-1.0.so*
%{_libdir}/libgsttag-1.0.so.*
%{_libdir}/libgstvideo-1.0.so.*
%{_libdir}/libgstfft-1.0.so.*
%{_libdir}/libgstrtsp-1.0.so.*
%{_libdir}/libgstsdp-1.0.so.*
%{_libdir}/libgstapp-1.0.so.*
%{_libdir}/libgstallocators-1.0.so.*

# base plugins without external dependencies
%{_libdir}/gstreamer-1.0/libgstadder.so
%{_libdir}/gstreamer-1.0/libgstaudioconvert.so
%{_libdir}/gstreamer-1.0/libgstplayback.so
%{_libdir}/gstreamer-1.0/libgsttypefindfunctions.so
%{_libdir}/gstreamer-1.0/libgstvideotestsrc.so
%{_libdir}/gstreamer-1.0/libgstvolume.so
%{_libdir}/gstreamer-1.0/libgstvideoconvert.so
%{_libdir}/gstreamer-1.0/libgstvideorate.so
%{_libdir}/gstreamer-1.0/libgstvideoscale.so
%{_libdir}/gstreamer-1.0/libgsttcp.so
%{_libdir}/gstreamer-1.0/libgstaudioresample.so
%{_libdir}/gstreamer-1.0/libgstaudiotestsrc.so
%{_libdir}/gstreamer-1.0/libgstapp.so
%exclude %{_libdir}/gstreamer-1.0/libgstencodebin.so
%{_libdir}/gstreamer-1.0/libgstsubparse.so
%{_libdir}/gstreamer-1.0/libgstximagesink.so

# base plugins with dependencies
%{_libdir}/gstreamer-1.0/libgstalsa.so
%{_libdir}/gstreamer-1.0/libgsttheora.so
%{_libdir}/gstreamer-1.0/libgstvorbis.so
%{_libdir}/gstreamer-1.0/libgstogg.so
%{_libdir}/gstreamer-1.0/libgstxvimagesink.so
%{_libdir}/gstreamer-1.0/libgstgio.so

%exclude %{_datadir}/gst-plugins-base/1.0/license-translations.dict

/usr/share/license/%{name}


%files devel
%manifest gst-plugins-base-devel.manifest
%defattr(-,root,root,-)
# plugin helper library headers
%{_includedir}/gstreamer-1.0/gst/audio/audio-channels.h
%{_includedir}/gstreamer-1.0/gst/audio/audio-format.h
%{_includedir}/gstreamer-1.0/gst/audio/audio-info.h
%{_includedir}/gstreamer-1.0/gst/video/gstvideodecoder.h
%{_includedir}/gstreamer-1.0/gst/video/gstvideoencoder.h
%{_includedir}/gstreamer-1.0/gst/video/gstvideoutils.h
%{_includedir}/gstreamer-1.0/gst/video/navigation.h
%{_includedir}/gstreamer-1.0/gst/video/video-blend.h
%{_includedir}/gstreamer-1.0/gst/video/video-color.h
%{_includedir}/gstreamer-1.0/gst/video/video-event.h
%{_includedir}/gstreamer-1.0/gst/video/video-format.h
%{_includedir}/gstreamer-1.0/gst/video/video-frame.h
%{_includedir}/gstreamer-1.0/gst/video/video-info.h
%{_includedir}/gstreamer-1.0/gst/video/video-overlay-composition.h
%{_includedir}/gstreamer-1.0/gst/rtp/gstrtphdrext.h

%{_includedir}/gstreamer-1.0/gst/audio/audio.h
%{_includedir}/gstreamer-1.0/gst/audio/gstaudiofilter.h
%{_includedir}/gstreamer-1.0/gst/riff/riff-ids.h
%{_includedir}/gstreamer-1.0/gst/riff/riff-media.h
%{_includedir}/gstreamer-1.0/gst/riff/riff-read.h
%{_includedir}/gstreamer-1.0/gst/video/gstvideopool.h
%{_includedir}/gstreamer-1.0/gst/video/gstvideometa.h
%{_includedir}/gstreamer-1.0//gst/audio/gstaudiometa.h
%{_includedir}/gstreamer-1.0/gst/audio/gstaudiosrc.h
%{_includedir}/gstreamer-1.0/gst/rtp/gstrtpbuffer.h
%{_includedir}/gstreamer-1.0/gst/audio/gstaudioclock.h
%{_includedir}/gstreamer-1.0/gst/audio/gstaudiosink.h
%{_includedir}/gstreamer-1.0/gst/video/gstvideofilter.h
%{_includedir}/gstreamer-1.0/gst/video/gstvideosink.h
%{_includedir}/gstreamer-1.0/gst/video/video-tile.h
%{_includedir}/gstreamer-1.0/gst/audio/gstaudiocdsrc.h
%{_includedir}/gstreamer-1.0/gst/pbutils/descriptions.h
%{_includedir}/gstreamer-1.0/gst/pbutils/install-plugins.h
%{_includedir}/gstreamer-1.0/gst/pbutils/missing-plugins.h
%{_includedir}/gstreamer-1.0/gst/rtp/gstrtcpbuffer.h
%{_includedir}/gstreamer-1.0/gst/fft/gstfft.h
%{_includedir}/gstreamer-1.0/gst/fft/gstfftf32.h
%{_includedir}/gstreamer-1.0/gst/fft/gstfftf64.h
%{_includedir}/gstreamer-1.0/gst/fft/gstffts16.h
%{_includedir}/gstreamer-1.0/gst/fft/gstffts32.h
%{_includedir}/gstreamer-1.0/gst/rtsp/gstrtsp-enumtypes.h
%{_includedir}/gstreamer-1.0/gst/rtsp/gstrtspconnection.h
%{_includedir}/gstreamer-1.0/gst/rtsp/gstrtspdefs.h
%{_includedir}/gstreamer-1.0/gst/rtsp/gstrtspextension.h
%{_includedir}/gstreamer-1.0/gst/rtsp/gstrtspmessage.h
%{_includedir}/gstreamer-1.0/gst/rtsp/gstrtsprange.h
%{_includedir}/gstreamer-1.0/gst/rtsp/gstrtsptransport.h
%{_includedir}/gstreamer-1.0/gst/rtsp/gstrtspurl.h
%{_includedir}/gstreamer-1.0/gst/sdp/gstsdp.h
%{_includedir}/gstreamer-1.0/gst/sdp/gstsdpmessage.h
%{_includedir}/gstreamer-1.0/gst/sdp/gstmikey.h
%{_includedir}/gstreamer-1.0/gst/rtp/gstrtppayloads.h
%{_includedir}/gstreamer-1.0/gst/tag/gsttagdemux.h
%{_includedir}/gstreamer-1.0/gst/pbutils/pbutils-enumtypes.h
%{_includedir}/gstreamer-1.0/gst/app/gstappsink.h
%{_includedir}/gstreamer-1.0/gst/app/gstappsrc.h
%{_includedir}/gstreamer-1.0/gst/audio/audio-enumtypes.h
%{_includedir}/gstreamer-1.0/gst/video/video-enumtypes.h
%{_includedir}/gstreamer-1.0/gst/pbutils/codec-utils.h
%{_includedir}/gstreamer-1.0/gst/pbutils/encoding-profile.h
%{_includedir}/gstreamer-1.0/gst/pbutils/encoding-target.h
%{_includedir}/gstreamer-1.0/gst/pbutils/gstdiscoverer.h
%{_includedir}/gstreamer-1.0/gst/pbutils/gstpluginsbaseversion.h
%{_includedir}/gstreamer-1.0/gst/tag/xmpwriter.h
%{_includedir}/gstreamer-1.0/gst/audio/gstaudioiec61937.h
%{_includedir}/gstreamer-1.0/gst/audio/gstaudiodecoder.h
%{_includedir}/gstreamer-1.0/gst/audio/gstaudioencoder.h
%{_includedir}/gstreamer-1.0/gst/tag/gsttagmux.h
%{_includedir}/gstreamer-1.0/gst/audio/gstaudiobasesink.h
%{_includedir}/gstreamer-1.0/gst/audio/gstaudiobasesrc.h
%{_includedir}/gstreamer-1.0/gst/audio/gstaudioringbuffer.h
%{_includedir}/gstreamer-1.0/gst/rtp/gstrtpbaseaudiopayload.h
%{_includedir}/gstreamer-1.0/gst/rtp/gstrtpbasedepayload.h
%{_includedir}/gstreamer-1.0/gst/rtp/gstrtpbasepayload.h
%{_includedir}/gstreamer-1.0/gst/audio/streamvolume.h
%{_includedir}/gstreamer-1.0/gst/video/colorbalance.h
%{_includedir}/gstreamer-1.0/gst/video/colorbalancechannel.h
%{_includedir}/gstreamer-1.0/gst/video/videoorientation.h
%{_includedir}/gstreamer-1.0/gst/video/videooverlay.h

%{_includedir}/gstreamer-1.0/gst/video/video-chroma.h

%{_includedir}/gstreamer-1.0/gst/video/cameracontrol.h
%{_includedir}/gstreamer-1.0/gst/video/cameracontrolchannel.h
%{_includedir}/gstreamer-1.0/gst/video/propertyprobe.h

%{_includedir}/gstreamer-1.0/gst/app/app.h
%{_includedir}/gstreamer-1.0/gst/pbutils/pbutils.h
%{_includedir}/gstreamer-1.0/gst/riff/riff.h
%{_includedir}/gstreamer-1.0/gst/rtp/rtp.h
%{_includedir}/gstreamer-1.0/gst/rtsp/rtsp.h
%{_includedir}/gstreamer-1.0/gst/tag/tag.h
%{_includedir}/gstreamer-1.0/gst/video/video.h
%{_includedir}/gstreamer-1.0/gst/fft/fft.h
%{_includedir}/gstreamer-1.0/gst/rtsp/gstrtsp.h
%{_includedir}/gstreamer-1.0/gst/sdp/sdp.h
%{_includedir}/gstreamer-1.0/gst/allocators/allocators.h
%{_includedir}/gstreamer-1.0/gst/allocators/gstdmabuf.h
%{_libdir}/libgstallocators-1.0.so
%{_libdir}/pkgconfig/gstreamer-allocators-1.0.pc

%{_libdir}/libgstfft-1.0.so
%{_libdir}/libgstrtsp-1.0.so
%{_libdir}/libgstsdp-1.0.so
%{_libdir}/libgstaudio-1.0.so
%{_libdir}/libgstriff-1.0.so
%{_libdir}/libgsttag-1.0.so
%{_libdir}/libgstvideo-1.0.so
%{_libdir}/libgstrtp-1.0.so
%{_libdir}/libgstpbutils-1.0.so
%{_libdir}/libgstapp-1.0.so

# pkg-config files
%{_libdir}/pkgconfig/gstreamer-plugins-base-1.0.pc
%{_libdir}/pkgconfig/gstreamer-audio-1.0.pc
%{_libdir}/pkgconfig/gstreamer-fft-1.0.pc
%{_libdir}/pkgconfig/gstreamer-pbutils-1.0.pc
%{_libdir}/pkgconfig/gstreamer-riff-1.0.pc
%{_libdir}/pkgconfig/gstreamer-rtp-1.0.pc
%{_libdir}/pkgconfig/gstreamer-rtsp-1.0.pc
%{_libdir}/pkgconfig/gstreamer-sdp-1.0.pc
%{_libdir}/pkgconfig/gstreamer-tag-1.0.pc
%{_libdir}/pkgconfig/gstreamer-video-1.0.pc
%{_libdir}/pkgconfig/gstreamer-app-1.0.pc

