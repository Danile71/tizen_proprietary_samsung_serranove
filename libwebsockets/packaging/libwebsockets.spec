Name:       libwebsockets
Summary:    WebSocket Library
Version:    1.2
Release:    1
Group:      System/Libraries
License:    LGPL-2.1+
URL:        http://git.warmcat.com/cgi-bin/cgit/libwebsockets/
Source0:    %{name}-%{version}.tar.bz2
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires: zlib-devel
BuildRequires: openssl-devel
BuildRequires: cmake
BuildRequires: pkgconfig(libsystemd-daemon)

%define _optdeveldir /opt/usr/devel/usr/

%description
C Websockets Server Library

%package devel
Summary:    Development files for %{name}
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Development files needed for building websocket clients and servers

%prep
%setup -q -n %{name}-%{version}

%build

%cmake -DWITH_SSL=On -DWITH_SD_DAEMON=ON -DWITHOUT_TESTAPPS=ON

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}

%make_install
mkdir -p %{buildroot}%{_datadir}/license
install -m0644 %{_builddir}/%{buildsubdir}/LICENSE %{buildroot}%{_datadir}/license/%{name}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_libdir}/libwebsockets*.so.*
%{_datadir}/license/%{name}

%files devel
%defattr(-,root,root,-)
%{_includedir}/libwebsockets.h
%{_libdir}/libwebsockets.so
%{_libdir}/pkgconfig/*
