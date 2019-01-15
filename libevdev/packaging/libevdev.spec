Name:           libevdev
Version:        1.2.2
Release:        0
License:        MIT
Summary:        wrapper library for evdev input devices
Url:            git://anongit.freedesktop.org/libevdev
Group:          System/Libraries
Source:         %{name}-%{version}.tar.gz

BuildRequires:  doxygen
BuildRequires:  make
BuildRequires:  python


%description
libevdev is a wrapper library for evdev devices. it moves the common
tasks when dealing with evdev devices into a library and provides a library
interface to the callers, thus avoiding erroneous ioctls, etc.


%package devel
Summary:    wrapper library for evdev input devices
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
libevdev is a wrapper library for evdev devices. it moves the common
tasks when dealing with evdev devices into a library and provides a library
interface to the callers, thus avoiding erroneous ioctls, etc.

%prep
%setup -q
%autogen

%build
make %{?jobs:-j%jobs} V=1

%install
%make_install
#%fdupes %{buildroot}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%defattr(-,root,root)
%{_bindir}/touchpad-edge-detector
%{_libdir}/*.so.*
%{_datadir}/*


%files devel
%defattr(-,root,root,-)
%{_includedir}/*
%{_libdir}/*.so
%{_libdir}/pkgconfig/*
