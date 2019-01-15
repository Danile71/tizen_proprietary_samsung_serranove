%bcond_with libproxy
Name:           glib-networking
Version:        2.38.0
Release:        0
License:        LGPL-2.1+
Summary:        Network-related GIO modules for glib
Group:          System/Libraries
Source:         http://download.gnome.org/sources/glib-networking/2.35/%{name}-%{version}.tar.xz
Source99:       baselibs.conf
Source1001:     glib-networking.manifest
Url:            http://www.gnome.org
BuildRequires:  intltool
BuildRequires:  which
BuildRequires:  libgcrypt-devel
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(gio-2.0) >= 2.31.6
BuildRequires:  pkgconfig(gnutls) >= 2.11.0
%if %{with libproxy}
BuildRequires:  pkgconfig(libproxy-1.0)
%endif

%description
This package contains network-related GIO modules for glib.

Currently, there is only a proxy module based on libproxy.

%lang_package

%prep
%setup -q
cp %{SOURCE1001} .

%build
%autogen \
    --disable-static \
%if %{with libproxy}
    --with-libproxy  \
%endif
    --with-ca-certificates=/etc/ssl/ca-bundle.pem
%__make %{?_smp_mflags} V=1

%install
%make_install
%find_lang %{name}

%post
%glib2_gio_module_post

%postun
%glib2_gio_module_postun

%files
%manifest %{name}.manifest
%defattr(-, root, root)
%license COPYING
%{_libdir}/gio/modules/libgiognutls.so

%if %{with libproxy}
%{_libdir}/gio/modules/libgiolibproxy.so
%{_libexecdir}/glib-pacrunner
%{_datadir}/dbus-1/services/org.gtk.GLib.PACRunner.service
%endif
