%define _sbindir /sbin
%define _moduledir %{_libdir}/security
%define _secconfdir %{_sysconfdir}/security
%define _pamconfdir %{_sysconfdir}/pam.d

Name: pam
Version: 1.1.5
Release: 4
License: BSD and GPLv2+ and BSD with advertising
Group: System/Base
Summary: PAM
URL: http://www.linux-pam.org/
Source0: %{name}-%{version}.tar.bz2
Source1: packaging/system-auth
Source2: packaging/other
Source1001: packaging/%{name}.manifest

Requires(post): /sbin/ldconfig
Requires(post): /usr/bin/install
Requires(postun): /sbin/ldconfig
BuildRequires: db4-devel
BuildRequires: bison
BuildRequires: flex
BuildRequires: gcc
BuildRequires: zlib-devel
BuildRequires: net-tools

%description
PAM (Pluggable Authentication Modules) is a system security tool that
allows system administrators to set authentication policy without
having to recompile programs that handle authentication.


%package -n pam-modules-extra
Group: System/Base
Summary: Extra modules provided by PAM not used in the base system
Requires: pam = %{version}-%{release}

%description -n pam-modules-extra
PAM (Pluggable Authentication Modules) is a system security tool that
allows system administrators to set authentication policy without
having to recompile programs that handle authentication. This package
contains extra modules for use by programs that are not used in the
default Tizen install.


%package devel
Group: Development/Libraries
Summary: Files needed for developing PAM-aware applications and modules for PAM
Requires: pam = %{version}-%{release}

%description devel
PAM (Pluggable Authentication Modules) is a system security tool that
allows system administrators to set authentication policy without
having to recompile programs that handle authentication. This package
contains header files and static libraries used for building both
PAM-aware applications and modules for use with PAM.

%prep
%setup

libtoolize -f #--copy --force && aclocal && autoheader
autoreconf

%build
cp %{SOURCE1001} .
CFLAGS="-fPIC $RPM_OPT_FLAGS -fPIE " ; export CFLAGS

%configure \
	--libdir=%{_libdir} \
	--includedir=%{_includedir}/security \
	--enable-isadir=../..%{_moduledir} \
	--disable-audit \
    --disable-nls \
	--with-db-uniquename=_pam \
    --with-libiconv-prefix=/usr \
    --enable-read-both-confs &&

make %{?_smp_flags} CFLAGS="$CFLAGS -lfl -lcrypt -pie"

%install
%make_install

# RPM uses docs from source tree
rm -rf $RPM_BUILD_ROOT%{_datadir}/doc/Linux-PAM
# Included in setup package
rm -f $RPM_BUILD_ROOT%{_sysconfdir}/environment

for phase in auth acct passwd session ; do
	ln -sf pam_unix.so $RPM_BUILD_ROOT%{_moduledir}/pam_unix_${phase}.so
done

# Install default pam configuration files
install -d -m 0755 %{buildroot}%{_pamconfdir}
install -m 0644 %{SOURCE1} %{buildroot}%{_pamconfdir}/
install -m 0644 %{SOURCE2} %{buildroot}%{_pamconfdir}/

mkdir -p $RPM_BUILD_ROOT%{_datadir}/license
for keyword in LICENSE COPYING COPYRIGHT;
do
	for file in `find %{_builddir} -name $keyword`;
	do
		cat $file >> $RPM_BUILD_ROOT%{_datadir}/license/%{name};
		echo "";
	done;
done

%post
/sbin/ldconfig
if [ ! -a /var/log/faillog ] ; then
	/usr/bin/install -m 600 /dev/null /var/log/faillog
fi
if [ ! -a /var/log/tallylog ] ; then
	/usr/bin/install -m 600 /dev/null /var/log/tallylog
fi

%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%manifest pam.manifest
%doc Copyright
%{_datadir}/license/%{name}
%{_sbindir}/pam_tally
%{_sbindir}/pam_tally2
%exclude %attr(4755,root,root) %{_sbindir}/pam_timestamp_check
%exclude %attr(4755,root,root) %{_sbindir}/unix_chkpwd
%exclude %attr(0700,root,root) %{_sbindir}/unix_update
%exclude %attr(0755,root,root) %{_sbindir}/mkhomedir_helper
/etc/security/limits.conf
%{_libdir}/libpam.so.*
%{_libdir}/libpam_misc.so.*
%{_libdir}/libpamc.so.*
%dir %{_moduledir}
%{_moduledir}/pam_deny.so
%{_moduledir}/pam_env.so
%{_moduledir}/pam_keyinit.so
%{_moduledir}/pam_limits.so
%{_moduledir}/pam_loginuid.so
%{_moduledir}/pam_namespace.so
%{_moduledir}/pam_nologin.so
%{_moduledir}/pam_permit.so
%{_moduledir}/pam_rootok.so
%{_moduledir}/pam_securetty.so
%{_moduledir}/pam_succeed_if.so
%{_moduledir}/pam_unix.so
%{_moduledir}/pam_wheel.so
%{_moduledir}/pam_xauth.so
%{_moduledir}/pam_filter
%dir %{_secconfdir}
%config(noreplace) %{_secconfdir}/access.conf
%config(noreplace) %{_secconfdir}/group.conf
%config(noreplace) %{_secconfdir}/namespace.conf
%dir %{_secconfdir}/namespace.d
%attr(755,root,root) %config(noreplace) %{_secconfdir}/namespace.init
%config(noreplace) %{_secconfdir}/pam_env.conf
%config(noreplace) %{_secconfdir}/time.conf
%exclude /var/run/sepermit
%dir %{_pamconfdir}
%{_pamconfdir}/system-auth
%{_pamconfdir}/other

%files -n pam-modules-extra
%defattr(-,root,root,-)
%manifest pam.manifest
%{_datadir}/license/%{name}
%{_moduledir}/pam_access.so
%{_moduledir}/pam_debug.so
%{_moduledir}/pam_echo.so
%{_moduledir}/pam_exec.so
%{_moduledir}/pam_faildelay.so
%{_moduledir}/pam_filter.so
%{_moduledir}/pam_ftp.so
%{_moduledir}/pam_group.so
%{_moduledir}/pam_issue.so
%{_moduledir}/pam_lastlog.so
%{_moduledir}/pam_listfile.so
%{_moduledir}/pam_localuser.so
%{_moduledir}/pam_mail.so
%{_moduledir}/pam_mkhomedir.so
%{_moduledir}/pam_motd.so
%{_moduledir}/pam_pwhistory.so
%{_moduledir}/pam_rhosts.so
%{_moduledir}/pam_shells.so
%{_moduledir}/pam_stress.so
%{_moduledir}/pam_tally.so
%{_moduledir}/pam_time.so
%{_moduledir}/pam_timestamp.so
%{_moduledir}/pam_umask.so
%{_moduledir}/pam_unix_acct.so
%{_moduledir}/pam_unix_auth.so
%{_moduledir}/pam_unix_passwd.so
%{_moduledir}/pam_unix_session.so
%{_moduledir}/pam_warn.so

%files devel
%defattr(-,root,root)
%manifest pam.manifest
%{_datadir}/license/%{name}
%{_includedir}/security/*
%doc %{_mandir}/man3/*
%doc %{_mandir}/man5/*
%doc %{_mandir}/man8/*
%{_libdir}/libpam.so
%{_libdir}/libpam_misc.so
%{_libdir}/libpamc.so
%{_libdir}/security/pam_tally2.so


