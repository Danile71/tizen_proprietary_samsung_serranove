Name:              gstreamer1.0
Summary:           GStreamer streaming media framework runtime
Version:           1.4.5
Release:           13
Group:             Applications/Multimedia
License:           LGPL
Source0:           %{name}-%{version}.tar.gz
Source1:  	   gstreamer-registry.service
Source2:  	   gstreamer-registry-for-system.service
Requires(post):    /sbin/ldconfig
Requires(postun):  /sbin/ldconfig
BuildRequires:     gettext
BuildRequires:     pkgconfig(glib-2.0)
BuildRequires:     bison
BuildRequires:     flex
BuildRequires:     pkgconfig(dlog)
BuildRequires:     python

#BuildRequires:     pkgconfig(libxml-2.0)
#BuildRequires:     pkgconfig(mm-ta)


%description
GStreamer is a streaming media framework, based on graphs of filters which
operate on media data. Applications using this library can do anything
from real-time sound processing to playing videos, and just about anything
else media-related.  Its plugin-based architecture means that new data
types or processing capabilities can be added simply by installing new
plugins.


%package devel
Summary:    Libraries/include files for GStreamer streaming media framework
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
GStreamer is a streaming media framework, based on graphs of filters which
operate on media data. Applications using this library can do anything
from real-time sound processing to playing videos, and just about anything
else media-related.  Its plugin-based architecture means that new data
types or processing capabilities can be added simply by installing new
plugins.

This package contains the libraries and includes files necessary to develop
applications and plugins for GStreamer, as well as general and API
documentation.


%prep
%setup -q -n %{name}-%{version}


%build
./autogen.sh

export CFLAGS+=" -Wall -g -fPIC\
	-DGST_UPSTREAM\
	-DGST_BASEPARSE_MODIFICATION\
	-DGST_EXT_MODIFIED_DQBUF\
	-DGST_QUEUE_ENHANCEMENT\
	-DGST_QUEUE2_MODIFICATION\
	-DGST_TIZEN_MODIFICATION\
	-DGST_REGISTRY_MODIFICATION\
	-DFILESRC_MODIFICATION\
	-DGST_EXT_BASEPARSER_MODIFICATION\
	-DGST_ADD_LOG\
	-DGST_MQ_MODIFICATION\
	-DGST_BUG_PATCH"

%configure --prefix=/usr\
 --disable-nls\
 --disable-static\
 --disable-gobject-cast-checks\
 --enable-binary-registry\
 --disable-loadsave\
 --disable-trace\
 --disable-docbook\
 --disable-gtk-doc\
 --enable-dlog\
 --with-html-dir=/tmp/dump


make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp COPYING %{buildroot}/usr/share/license/%{name}
mkdir -p %{buildroot}%{_libdir}/systemd/system
install -m 644 %{SOURCE1} %{buildroot}%{_libdir}/systemd/system/gstreamer-registry.service
install -m 644 %{SOURCE2} %{buildroot}%{_libdir}/systemd/system/gstreamer-registry-for-system.service
mkdir -p  %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants
ln -s  ../gstreamer-registry.service  %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/gstreamer-registry.service
ln -s  ../gstreamer-registry-for-system.service  %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/gstreamer-registry-for-system.service
%make_install
rm -rf %{buildroot}/tmp/dump


%post
/sbin/ldconfig
mkdir -p /home/app/.cache/gstreamer-1.0
mkdir -p /home/system/.cache/gstreamer-1.0
chown app:app /home/app/.cache
chown app:app /home/app/.cache/gstreamer-1.0
chown system:system /home/system/.cache
chown system:system /home/system/.cache/gstreamer-1.0
chmod 755 /home/app/.cache/gstreamer-1.0
chmod 755 /home/system/.cache/gstreamer-1.0

mkdir -p %{_libdir}/gstreamer-1.0
chsmack -a "_" %{_libdir}/gstreamer-1.0

%postun -p /sbin/ldconfig


%files
%manifest gstreamer.manifest
%defattr(-,root,root,-)
%doc AUTHORS COPYING NEWS README RELEASE TODO
%{_libdir}/libgstreamer-1.0.so.*
%{_libdir}/libgstbase-1.0.so.*
%{_libdir}/libgstcontroller-1.0.so.*
%{_libdir}/libgstnet-1.0.so.*

%dir %{_libdir}/gstreamer-1.0
%{_libdir}/gstreamer-1.0/libgstcoreelements.so

%{_bindir}/gst-inspect-1.0
%{_bindir}/gst-launch-1.0
%{_bindir}/gst-typefind-1.0
%{_libexecdir}/gstreamer-1.0/gst-plugin-scanner
%doc %{_mandir}/man1/gst-inspect-1.0.*
%doc %{_mandir}/man1/gst-launch-1.0.*
%doc %{_mandir}/man1/gst-typefind-1.0.*
#%doc %{_datadir}/doc/gstreamer-1.0/manual
#%doc %{_datadir}/doc/gstreamer-1.0/pwg
#%doc %{_datadir}/doc/gstreamer-1.0/faq/
#%{_libdir}/girepository-1.0/Gst-1.0.typelib
#%{_libdir}/girepository-1.0/GstBase-1.0.typelib
#%{_libdir}/girepository-1.0/GstCheck-1.0.typelib
#%{_libdir}/girepository-1.0/GstController-1.0.typelib
#%{_libdir}/girepository-1.0/GstNet-1.0.typelib
#%{_datadir}/gir-1.0/Gst-1.0.gir
#%{_datadir}/gir-1.0/GstBase-1.0.gir
#%{_datadir}/gir-1.0/GstCheck-1.0.gir
#%{_datadir}/gir-1.0/GstController-1.0.gir
#%{_datadir}/gir-1.0/GstNet-1.0.gir
/usr/share/license/%{name}
%{_libdir}/systemd/system/gstreamer-registry.service
%{_libdir}/systemd/system/gstreamer-registry-for-system.service
%{_libdir}/systemd/system/multi-user.target.wants/gstreamer-registry.service
%{_libdir}/systemd/system/multi-user.target.wants/gstreamer-registry-for-system.service

%files devel
%manifest gstreamer-devel.manifest
%defattr(-,root,root,-)
%dir %{_includedir}/gstreamer-1.0
%dir %{_includedir}/gstreamer-1.0/gst
%{_includedir}/gstreamer-1.0/gst/*.h

%{_includedir}/gstreamer-1.0/gst/base
%{_includedir}/gstreamer-1.0/gst/check
%{_includedir}/gstreamer-1.0/gst/controller
%{_includedir}/gstreamer-1.0/gst/net

%{_libdir}/libgstreamer-1.0.so
%{_libdir}/libgstbase-1.0.so
%{_libdir}/libgstcheck-1.0.so*
%{_libdir}/libgstcontroller-1.0.so
%{_libdir}/libgstnet-1.0.so

%{_datadir}/aclocal/gst-element-check-1.0.m4
%{_libdir}/pkgconfig/gstreamer-1.0.pc
%{_libdir}/pkgconfig/gstreamer-base-1.0.pc
%{_libdir}/pkgconfig/gstreamer-controller-1.0.pc
%{_libdir}/pkgconfig/gstreamer-check-1.0.pc
%{_libdir}/pkgconfig/gstreamer-net-1.0.pc

#%doc %{_datadir}/gtk-doc/html/gstreamer-1.0/*
#%doc %{_datadir}/gtk-doc/html/gstreamer-libs-1.0/*
#%doc %{_datadir}/gtk-doc/html/gstreamer-plugins-1.0/*

