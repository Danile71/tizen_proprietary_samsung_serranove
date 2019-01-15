Summary: A tool for determining compilation options
Name: pkg-config
Version: 0.25
Release: %{?release_prefix:%{release_prefix}.}2.47.%{?dist}%{!?dist:tizen}
VCS:     external/pkg-config#Z910F_PROTEX_0625-2-g4d73d1879925f70f08441274250fa42f6c9ae71e
License: GPLv2+
URL: http://pkgconfig.freedesktop.org
Group: Development/Tools
Source:  http://www.freedesktop.org/software/pkgconfig/releases/pkg-config-%{version}.tar.gz
Patch1:  pkg-config-system_libdir-multilib.patch
#BuildRequires: glib2-devel
BuildRequires: popt-devel

Provides: pkgconfig
Provides: pkgconfig(pkg-config) = %{version}

%description
The pkgconfig tool determines compilation options. For each required
library, it reads the configuration file and outputs the necessary
compiler and linker flags.

%prep
%setup -n pkg-config-%{version} -q
%patch1 -p1

%build
%configure \
        --disable-shared \
%ifarch x86_64
        --with-pc-path=%{_prefix}/lib/pkgconfig:%{_libdir}/pkgconfig:%{_datadir}/pkgconfig
%else
        --with-pc-path=%{_libdir}/pkgconfig:%{_datadir}/pkgconfig
%endif
make %{?_smp_mflags}

%install
make install DESTDIR=$RPM_BUILD_ROOT
%ifarch x86_64
mkdir -p $RPM_BUILD_ROOT%{_prefix}/lib/pkgconfig
%endif
mkdir -p $RPM_BUILD_ROOT%{_libdir}/pkgconfig
mkdir -p $RPM_BUILD_ROOT%{_datadir}/pkgconfig

# we include this below, already
rm -rf $RPM_BUILD_ROOT%{_datadir}/doc/pkg-config

%remove_docs

mkdir -p %{buildroot}/usr/share/license
cp -f COPYING %{buildroot}/usr/share/license/%{name}

%files
%defattr(-,root,root)
%{_bindir}/*
%ifarch x86_64
%dir %{_prefix}/lib/pkgconfig
%endif
%dir %{_libdir}/pkgconfig
%dir %{_datadir}/pkgconfig
%{_datadir}/aclocal/*
/usr/share/license/%{name}
%manifest pkg-config.manifest

