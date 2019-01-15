Summary:		User space tools for 2.6 kernel auditing
Name:			audit
Version:		2.2.2
Release:		5
License:		GPLv2+
Group:			System Environment/Daemons
URL:			http://people.redhat.com/sgrubb/audit/
Source0:		http://people.redhat.com/sgrubb/audit/%{name}-%{version}.tar.gz
Patch1:         audit-no_plugins.patch
Patch2:         audit-no-gss.patch
Patch3:         audit-memory-leak-fix.patch
BuildRequires:  autoconf >= 2.12
BuildRequires:  kernel-headers >= 2.6.29
BuildRequires:  libtool
BuildRequires:  pkgconfig(systemd)
Requires:		%{name}-libs = %{version}-%{release}

%description
The audit package contains the user space utilities for
storing and searching the audit records generate by
the audit subsystem in the Linux 2.6 kernel.

%package libs
Summary: Dynamic library for libaudit
License: LGPLv2+
Group: Development/Libraries

%description libs
The libaudit package contains the dynamic libraries needed for
applications to use the audit framework.

%package devel
Summary:		Header files and static library for libaudit
License:		LGPLv2+
Group:			Development/Libraries
Requires:		%{name}-libs = %{version}-%{release}

%description devel
The audit-devel package contains the static libraries and header 
files needed for developing applications that need to use the audit 
framework libraries.

%prep
%setup -q
%patch1 -p1
%patch2 -p1
%patch3 -p1

%build
autoreconf -fi

%configure --enable-systemd --disable-static --with-pic --without-python
make %{?_smp_mflags}

%install
mkdir -p $RPM_BUILD_ROOT/{sbin,etc/{sysconfig,audispd/plugins.d,init.d}}
mkdir -p $RPM_BUILD_ROOT/usr/sbin
mkdir -p $RPM_BUILD_ROOT/%{_lib}/security
make DESTDIR=$RPM_BUILD_ROOT install
mkdir -p $RPM_BUILD_ROOT/%{_includedir}
mkdir -p $RPM_BUILD_ROOT/%{_libdir}
# We manually install this since Makefile doesn't
install -m 0644 lib/libaudit.h $RPM_BUILD_ROOT/%{_includedir}

for libname in libaudit libauparse;do
  rm -v %{buildroot}/%{_libdir}/$libname.la
done

ln -s %{_prefix}/sbin/auditd %{buildroot}/sbin/auditd
ln -s %{_prefix}/sbin/audispd %{buildroot}/sbin/audispd
ln -s %{_prefix}/sbin/auditctl %{buildroot}/sbin/auditctl

mkdir -p $RPM_BUILD_ROOT/opt/var/log/audit/
mkdir -p $RPM_BUILD_ROOT/opt/var/spool/audit/
# On platforms with 32 & 64 bit libs, we need to coordinate the timestamp
touch -r ./audit.spec $RPM_BUILD_ROOT/etc/libaudit.conf

mkdir -p %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants
ln -s %{_unitdir}/auditd.service %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/auditd.service

mkdir -p %{buildroot}/usr/share/license
cp COPYING %{buildroot}/usr/share/license/%{name}

%remove_docs

#%check
#make check

%post libs -p /sbin/ldconfig

%postun libs -p /sbin/ldconfig

%post

%preun

%postun

%files libs
%defattr(-,root,root)
%{_libdir}/libaudit.so.*
%{_libdir}/libauparse.so.*
%config(noreplace) %attr(640,root,root) /etc/libaudit.conf

%files devel
%defattr(-,root,root)
%doc contrib/skeleton.c contrib/plugin
%{_libdir}/libaudit.so
%{_libdir}/libauparse.so
%{_includedir}/libaudit.h
%{_includedir}/auparse.h
%{_includedir}/auparse-defs.h

%files
%manifest audit.manifest
%defattr(-,root,root,-)
%doc  README COPYING ChangeLog contrib/capp.rules contrib/nispom.rules contrib/lspp.rules contrib/stig.rules init.d/auditd.cron
%attr(750,root,root) /usr/sbin/auditctl
%attr(750,root,root) /sbin/auditctl
%attr(750,root,root) /usr/sbin/auditd
%attr(750,root,root) /sbin/auditd
%attr(755,root,root) /usr/sbin/ausearch
%attr(750,root,root) /usr/sbin/autrace
%attr(750,root,root) /usr/sbin/audispd
%attr(750,root,root) /sbin/audispd
%attr(755,root,root) /usr/bin/aulast
%attr(755,root,root) /usr/bin/aulastlog
%attr(755,root,root) /usr/bin/ausyscall
%attr(755,root,root) /usr/sbin/aureport
%attr(755,root,root) /usr/bin/auvirt
%dir %attr(750,root,root) /etc/audit
%attr(750,root,root) %dir /etc/audisp
%attr(750,root,root) %dir /etc/audisp/plugins.d
%config(noreplace) %attr(640,root,root) /etc/audisp/plugins.d/af_unix.conf
%config(noreplace) %attr(640,root,root) /etc/audisp/plugins.d/syslog.conf
%config(noreplace) %attr(640,root,root) /etc/audit/auditd.conf
%config(noreplace) %attr(640,root,root) /etc/audit/audit.rules
%config(noreplace) %attr(640,root,root) /etc/audisp/audispd.conf
%dir %attr(755,root,root) /opt/var/log/audit
%dir %attr(700,root,root) /opt/var/spool/audit
%{_unitdir}/auditd.service
%{_libdir}/systemd/system/multi-user.target.wants/auditd.service
/usr/share/license/audit

%changelog
