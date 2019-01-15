Name: gupnp
Version: 0.14.1_5
Release: 1
Summary: GUPNP
Group: <group>/<group>
License: LGPL v2
URL: http://www.gupnp.org/
Source0: %{name}-%{version}.tar.gz
Patch1: missed_service_node_bug.patch
BuildRequires:  pkgconfig(glib-2.0) >= 2.18
BuildRequires:  pkgconfig(gobject-2.0) >= 2.18
BuildRequires:  pkgconfig(gmodule-2.0)
BuildRequires:  pkgconfig(gssdp-1.0) >= 0.7.1
BuildRequires:  pkgconfig(libsoup-2.4) >= 2.4.1
BuildRequires:	pkgconfig(libxml-2.0)
# need dbus-glib-1 for use network-manager
BuildRequires:	pkgconfig(dbus-glib-1) >= 0.76
BuildRequires:	pkgconfig(uuid)
BuildRequires:	pkgconfig(gthread-2.0)
BuildRequires:  autoconf >= 2.67
%description
GUPnP is an object-oriented open source framework for creating UPnP devices and
control points, written in C using GObject and libsoup. The GUPnP API is
intended to be easy to use, efficient and flexible.

%package devel
Summary:    GUPNP (devel)
Group:      Development/Headers
Requires:   %{name} = %{version}-%{release}

%description devel
GUPnP is an object-oriented open source framework for creating UPnP devices and
control points, written in C using GObject and libsoup. The GUPnP API is
intended to be easy to use, efficient and flexible.

%prep  
%setup -q  
%patch1 -p1
  
%build  
%configure --prefix=/usr --with-context-manager=network-manager
  
make %{?jobs:-j%jobs}  
  
%install  
rm -rf %{buildroot}  
%make_install
rm -rf %{buildroot}/usr/share/
mkdir -p %{buildroot}/usr/share/license
cp COPYING %{buildroot}/usr/share/license/%{name}

%clean
rm -rf %{buildroot}

%post

%postun

%files
%defattr(-,root,root,-)
%doc
/usr/lib/*.so.*
/usr/share/license/%{name}

%files devel
%defattr(-,root,root,-)
/usr/bin/gupnp-binding-tool
/usr/include/gupnp-1.0/*
#/usr/lib/*.a
/usr/lib/*.so
/usr/lib/pkgconfig/*

