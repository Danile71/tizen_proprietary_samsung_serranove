Name:       gst-plugins-camera-msm8916
Summary:    Gstreamer camera plugin package for MSM 8916
Version:    0.0.26
Release:    0
Group:      libs
License:    TO_BE_FILL
Source0:    %{name}-%{version}.tar.gz
ExclusiveArch:  %arm
#Requires(post): /usr/bin/vconftool
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(gstreamer-plugins-base-1.0)
BuildRequires:  pkgconfig(gstreamer-1.0)
BuildRequires:  gstreamer-plugins-base1.0-devel
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(camsrcjpegenc)
BuildRequires:  pkgconfig(qcom-camera)
BuildRequires:  pkgconfig(dlog)
#BuildRequires:  gstreamer-devel
#!BuildIgnore: kernel-headers
BuildRequires:  kernel-headers-tizen-dev
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(libtbm)
BuildRequires:  pkgconfig(mmutil-imgp)
BuildRequires:  pkgconfig(mmutil-jpeg)
BuildRequires:  pkgconfig(libdri2)
BuildRequires:  pkgconfig(xfixes)
BuildRequires:  pkgconfig(sensor)
BuildRequires:  pkgconfig(mmutil-imgp)
BuildRequires:  pkgconfig(mmutil-jpeg)
BuildRequires:  pkgconfig(csc-feature)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(libdrm)
#BuildRequires:  pkgconfig(vconf)

Provides: libqcamhal.so
Provides: libcamerahdr.so

%description
Gstreamer codec plugins package for MSM series
 GStreamer is a streaming media framework, based on graphs of filters
 which operate on media data. Multimedia Framework using this plugins
 library can encode and decode video, audio, and speech..

%prep
%setup -q

%build
%if 0%{?sec_build_binary_debug_enable}
export CFLAGS+=" -DGST_EXT_TIME_ANALYSIS -DTIZEN_DEBUG_ENABLE -include stdint.h"
export CXXFLAGS+=" -DTIZEN_DEBUG_ENABLE"
%else
export CFLAGS+=" -DGST_EXT_TIME_ANALYSIS -include stdint.h"
%endif

sh ./autogen.sh
%configure --disable-static --enable-debug=yes --enable-target=msm8974
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp LICENSE.Apache-2.0 %{buildroot}/usr/share/license/%{name}
cat LICENSE.BSD >>%{buildroot}/usr/share/license/%{name}
cat COPYING >>%{buildroot}/usr/share/license/%{name}
%make_install

%post
/sbin/ldconfig


%postun -p /sbin/ldconfig

%files
%manifest gst-plugins-camera-msm8916.manifest
%defattr(-,root,root,-)
%{_libdir}/gstreamer-1.0/lib*.so*
%{_libdir}/libqcamhal.so*
%{_libdir}/libcamerahdr.so*
%{_datadir}/license/%{name}

