Name:           libusb
Version:        1.0.9
Release:        0
License:        LGPL-2.1+
Summary:        USB Library
Url:            http://www.libusb.org/
Group:          Base/Device Management
Source:         %{name}-%{version}.tar.bz2
Source1:        baselibs.conf
Source1001: 	libusb.manifest
BuildRequires:  pkg-config

%description
Libusb is a library that allows userspace access to USB devices.

%package devel
Summary:        USB Library
Group:          Development/Libraries
Requires:       glibc-devel
Requires:       libusb = %{version}

%description devel
Libusb is a library that allows userspace access to USB devices.

%prep
%setup -q
cp %{SOURCE1001} .

%build
%configure\
	--with-pic\
	--disable-static
make %{?_smp_mflags}

%install
%make_install

%post  -p /sbin/ldconfig

%postun  -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%defattr(-,root,root)
%license COPYING
%{_libdir}/*.so.*

%files devel
%manifest %{name}.manifest
%defattr(-,root,root)
%{_includedir}/libusb-1.0
%{_libdir}/*.so
%{_libdir}/pkgconfig/*.pc

%changelog
