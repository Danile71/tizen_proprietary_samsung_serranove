Name:		dbus
Summary:	D-Bus message bus
Version:	1.8.18
Release:	1
Group:		System/Libraries
License:	GPLv2+ or AFL
URL:		http://www.freedesktop.org/software/dbus/
Source0:	http://dbus.freedesktop.org/releases/%{name}/%{name}-%{version}.tar.gz
Source1001:	dbus.manifest

Requires:	%{name}-libs = %{version}

BuildRequires:  expat-devel >= 1.95.5
BuildRequires:  libtool
BuildRequires:  which
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(libsmack)


%description
D-Bus is a system for sending messages between applications. It is used both
for the systemwide message bus service, and as a per-user-login-session
messaging facility.


%package libs
Summary:    Libraries for accessing D-Bus
Group:      System/Libraries
#FIXME: This is circular dependency
Requires:   %{name} = %{version}-%{release}

%description libs
Lowlevel libraries for accessing D-Bus.

%package devel
Summary:    Libraries and headers for D-Bus
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   pkgconfig

%description devel
Headers and static libraries for D-Bus.

%prep
%setup -q -n %{name}-%{version}

%build
cp %{SOURCE1001} .

export CFLAGS+=" -fpie"
export LDFLAGS+=" -pie"

./autogen.sh --prefix=/usr
%reconfigure \
    --disable-static \
    --bindir=%{_bindir} \
    --libdir=%{_libdir} \
    --sysconfdir=%{_sysconfdir} \
    --libexecdir=%{_libdir}/dbus-1 \
    --disable-asserts \
    --disable-verbose-mode \
    --disable-xml-docs \
    --disable-selinux \
    --disable-libaudit \
    --enable-smack \
    --disable-libaudit \
    --enable-tests=no \
    --with-system-pid-file=%{_localstatedir}/run/messagebus.pid \
    --with-dbus-user=root \
    --with-systemdsystemunitdir=%{_libdir}/systemd/system \
    --enable-stats \

make %{?jobs:-j%jobs}

%install
%make_install
%remove_docs

mkdir -p %{buildroot}%{_libdir}/pkgconfig
# Change the arch-deps.h include directory to /usr/lib instead of /lib
sed -e 's@-I${libdir}@-I${prefix}/%{_lib}@' %{buildroot}%{_libdir}/pkgconfig/dbus-1.pc

mkdir -p %{buildroot}%{_datadir}/dbus-1/interfaces

# We will not allow shipping dbus-monitor on user image.
%if ! 0%{?tizen_build_binary_release_type_eng:1}
rm -fr $RPM_BUILD_ROOT%{_bindir}/dbus-monitor
%endif

mkdir -p $RPM_BUILD_ROOT%{_datadir}/license
cat COPYING > $RPM_BUILD_ROOT%{_datadir}/license/dbus
cat COPYING > $RPM_BUILD_ROOT%{_datadir}/license/dbus-libs

%post
mkdir -p /var/lib/dbus
ln -sf %{_sysconfdir}/machine-id /var/lib/dbus/machine-id

%post libs -p /sbin/ldconfig

%postun libs -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_datadir}/license/dbus
%{_bindir}/dbus-cleanup-sockets
%{_bindir}/dbus-daemon
%if 0%{?tizen_build_binary_release_type_eng:1}
%{_bindir}/dbus-monitor
%endif
%{_bindir}/dbus-send
%{_bindir}/dbus-uuidgen
%{_bindir}/dbus-launch
%{_bindir}/dbus-run-session
%{_bindir}/dbus-test-tool
%dir %{_sysconfdir}/dbus-1
%config(noreplace) %{_sysconfdir}/dbus-1/session.conf
%dir %{_sysconfdir}/dbus-1/session.d
%config(noreplace) %{_sysconfdir}/dbus-1/system.conf
%dir %{_sysconfdir}/dbus-1/system.d
%dir %{_libdir}/dbus-1
%attr(4750,root,dbus) %{_libdir}/dbus-1/dbus-daemon-launch-helper
%{_libdir}/systemd/system/dbus.socket
%{_libdir}/systemd/system/dbus.target.wants/dbus.socket
%{_libdir}/systemd/system/sockets.target.wants/dbus.socket
%{_libdir}/systemd/system/dbus.service
%{_libdir}/systemd/system/multi-user.target.wants/dbus.service
%dir %{_datadir}/dbus-1
%{_datadir}/dbus-1/interfaces
%{_datadir}/dbus-1/services
%{_datadir}/dbus-1/system-services
%dir %{_localstatedir}/run/dbus
%dir %{_localstatedir}/lib/dbus
%manifest dbus.manifest

%files libs
%defattr(-,root,root,-)
%{_datadir}/license/dbus-libs
%{_libdir}/libdbus-1.so.3*

%files devel
%defattr(-,root,root,-)
%{_libdir}/libdbus-1.so
%{_includedir}/dbus-1.0/dbus/dbus*.h
%dir %{_libdir}/dbus-1.0
%{_libdir}/dbus-1.0/include/dbus/dbus-arch-deps.h
%{_libdir}/pkgconfig/dbus-1.pc
