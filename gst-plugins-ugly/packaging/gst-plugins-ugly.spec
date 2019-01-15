Name:       gst-plugins-ugly1.0
Summary:    GStreamer plugins from the "ugly" set
Version:    1.4.6
Release:    4
Group:      Applications/Multimedia
License:    LGPL
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(gstreamer-1.0)
BuildRequires:  gstreamer-plugins-base1.0-devel
BuildRequires:  gettext-tools
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  python
BuildRequires:  pkgconfig(opencore-amrnb)
BuildRequires:  pkgconfig(opencore-amrwb)
#BuildRequires:  sec-product-features

#BuildRequires:  which
#BuildRequires:  pkgconfig(liboil-0.3)
#BuildRequires:  pkgconfig(drm-client)
#BuildRequires:  pkgconfig(drm-trusted)


%description
GStreamer is a streaming media framework, based on graphs of elements which
operate on media data.

This package contains well-written plug-ins that can't be shipped in
gstreamer-plugins-good because:
- the license is not LGPL
- the license of the library is not LGPL
- there are possible licensing issues with the code.



%prep
%setup -q


%build
%autogen --noconfigure

export CFLAGS+=" -DASFDEMUX_MODIFICATION"

%configure --prefix=/usr\
 --disable-nls\
 --disable-static\
%if 0%{?sec_product_feature_mmfw_tz_enable}
# --enable-asfdemux_enable_playready \
%endif
 --with-html-dir=/tmp/dump\
 --disable-sidplay\
 --disable-mad\
 --disable-a52dec\
 --disable-dvdread\
 --disable-mpeg2dec\
 --disable-twolame\
 --disable-x264\
 --disable-drm-decryption\
 --disable-cdio

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
%manifest gst-plugins-ugly.manifest
%defattr(-,root,root,-)
%doc AUTHORS COPYING README REQUIREMENTS gst-plugins-ugly.doap
%{_libdir}/gstreamer-1.0/libgstasf.so
%{_libdir}/gstreamer-1.0/libgstdvdlpcmdec.so
%{_libdir}/gstreamer-1.0/libgstxingmux.so
%{_libdir}/gstreamer-1.0/libgstrmdemux.so
%{_libdir}/gstreamer-1.0/libgstdvdsub.so

# plugins with dependencies
#%{_libdir}/gstreamer-1.0/libgstsid.so
#%{_libdir}/gstreamer-1.0/libgstlame.so
#%{_libdir}/gstreamer-1.0/libgstmad.so
#%{_libdir}/gstreamer-1.0/libgsta52dec.so
#%{_libdir}/gstreamer-1.0/libgstdvdread.so
#%{_libdir}/gstreamer-1.0/libgstmpeg2dec.so
#%{_libdir}/gstreamer-1.0/libgsttwolame.so
#%doc %{_datadir}/gtk-doc/html/gst-plugins-ugly-plugins-1.0/*
#%{_datadir}/gstreamer-1.0/presets/GstX264Enc.prs
#%{_libdir}/gstreamer-1.0/libgstx264.so
%{_libdir}/gstreamer-1.0/libgstamrnb.so
%{_libdir}/gstreamer-1.0/libgstamrwbdec.so
%{_datadir}/gstreamer-1.0/presets/GstAmrnbEnc.prs
#%{_libdir}/gstreamer-1.0/libgstcdio.so
/usr/share/license/%{name}
