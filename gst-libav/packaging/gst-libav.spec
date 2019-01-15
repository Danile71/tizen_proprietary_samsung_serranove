Name:       gst-libav
Summary:    GStreamer Streaming-media framework plug-in using libav (FFmpeg).
Version:    1.4.6
Release:    2
Group:      Libraries/Multimedia
License:    LGPL
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(gstreamer-1.0)
BuildRequires:  pkgconfig(gstreamer-plugins-base-1.0)
BuildRequires:  pkgconfig(zlib)
BuildRequires:  pkgconfig(vorbis)
BuildRequires:  pkgconfig(theora)
BuildRequires:  python

#BuildRequires:  gettext
#BuildRequires:  which
#BuildRequires:  pkgconfig(glib-2.0)


%description
GStreamer is a streaming-media framework, based on graphs of filters which
operate on media data. Applications using this library can do anything
from real-time sound processing to playing videos, and just about anything
else media-related. Its plugin-based architecture means that new data
types or processing capabilities can be added simply by installing new
plug-ins.

This plugin contains the libav (formerly FFmpeg) codecs, containing codecs for most popular
multimedia formats.


%prep
%setup -q -n gst-libav-%{version}


%build


%configure --prefix=/usr\
 --disable-static --disable-gpl --enable-lgpl --with-libav-extra-configure="--disable-yasm --enable-static --enable-pic --enable-optimizations \
							  --disable-doc --disable-hwaccels\
							  --disable-gpl  --disable-postproc --disable-swscale  \
							  --disable-mmx --enable-neon --disable-dxva2 --disable-vdpau \
							  --disable-avconv --disable-avprobe --disable-avserver --disable-avplay   \
							  --disable-decoders --disable-encoders \
							  --disable-muxers --disable-demuxers \
							  --disable-parsers  \
							  --disable-protocols --disable-network --disable-bsfs --disable-devices --disable-filters  \
							  --enable-libvorbis --enable-libtheora \
							  --enable-muxer=amr \
							  --enable-muxer=tgp \
							  --enable-muxer=mp4 \
							  --enable-demuxer=aac   \
							  --enable-demuxer=ac3   \
							  --enable-demuxer=avi   \
							  --enable-demuxer=amr   \
							  --enable-demuxer=h263    \
							  --enable-demuxer=h264  \
							  --enable-demuxer=m4v   \
							  --enable-demuxer=mov   \
							  --enable-demuxer=mp3   \
							  --enable-demuxer=mpegts    \
							  --enable-demuxer=mpegps    \
							  --enable-demuxer=mpegtsraw \
							  --enable-demuxer=mpegvideo \
							  --enable-parser=aac    \
							  --enable-parser=ac3    \
							  --enable-parser=h263   \
							  --enable-parser=h264   \
							  --enable-parser=mpeg4video \
							  --enable-parser=mpegaudio  \
							  --enable-parser=mpegvideo"

make

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp COPYING %{buildroot}/usr/share/license/%{name}
%make_install
rm -f %{buildroot}{_libdir}/gstreamer-1.0/*.la
rm -f %{buildroot}{_libdir}/gstreamer-1.0/*.a

%clean
rm -rf %{buildroot}


%files
%manifest gst-libav.manifest
%defattr(-,root,root,-)
%doc AUTHORS COPYING README gst-libav.doap
#%{_datadir}/gtk-doc/html/gst-libav-plugins-%{version}/*
%{_libdir}/gstreamer-1.0/libgstlibav.so
#%{_libdir}/gstreamer-1.0/libgstpostproc.so
# disable/skip avvideoscale until someone makes it work - citation from gst-libav/ext/Makefile.am
#%{_libdir}/gstreamer-1.0/libgstavscale.so
/usr/share/license/%{name}
