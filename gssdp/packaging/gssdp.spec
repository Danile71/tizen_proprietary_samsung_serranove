Name: gssdp
Version: 0.8.2_2
Release: 1
Summary: GSSDP
Group: <group>/<group>
License: LGPL v2
URL: http://www.gupnp.org/
Source0: %{name}-%{version}.tar.gz
Patch1: specified_ssdp_addr.patch
Patch2: fixed_netmask_filter_bug.patch
BuildRequires:	pkgconfig(glib-2.0)
BuildRequires:	pkgconfig(libsoup-2.4)
BuildRequires:  autoconf >= 2.67
%description
A GObject-based API for handling resource discovery and announcement over SSDP.

%package devel
Summary:    GSSDP (devel)
Group:      Development/Headers
Requires:   %{name} = %{version}-%{release}
%description devel
A GObject-based API for handling resource discovery and announcement over SSDP.

%prep  
%setup -q  
%patch1 -p1 
%patch2 -p1

%build  
%configure --prefix=/usr --enable-introspection=no --enable-gtk-doc-html=no
  
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
%manifest %{name}.manifest
%defattr(-,root,root,-)
%doc
/usr/lib/*.so.*
/usr/share/license/%{name}

%files devel
%defattr(-,root,root,-)
/usr/include/gssdp-1.0/*
#/usr/lib/*.a
/usr/lib/*.so
/usr/lib/pkgconfig/*

