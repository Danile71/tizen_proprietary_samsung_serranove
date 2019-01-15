Name:       gst-plugins-tizen1.0
Version:    1.0.2
Summary:    GStreamer tizen plugins (common)
Release:    35
Group:      libs
License:    LGPLv2+
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(ecore-x)
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(gstreamer-video-1.0)
BuildRequires:  pkgconfig(gstreamer-plugins-base-1.0)
BuildRequires:  pkgconfig(gstreamer-1.0)  
BuildRequires:	pkgconfig(libdri2)
BuildRequires:	pkgconfig(x11)
BuildRequires:	pkgconfig(xext)
BuildRequires:	pkgconfig(xv)
BuildRequires:	pkgconfig(xdamage)
BuildRequires:	pkgconfig(libdrm)
BuildRequires:  pkgconfig(libtbm)
BuildRequires:	libdrm-devel

%if 0%{?sec_product_feature_profile_lite}
#!BuildIgnore:  kernel-headers
BuildRequires:  kernel-headers-tizen-dev
%endif

%description
GStreamer tizen plugins (common)

%prep
%setup -q


%build
export CFLAGS+=" -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" "

./autogen.sh --disable-static
%configure --disable-static 

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install


%files
%manifest gst-plugins-tizen1.0.manifest
%defattr(-,root,root,-)
%{_libdir}/libgstwfdbase.so*
%{_libdir}/gstreamer-1.0/*.so
