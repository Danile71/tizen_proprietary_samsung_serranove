Summary: A set of system configuration and setup files
Name: setup
Version: 0.1
Release: 4
License: Public Domain
Group: System/Base
URL: https://fedorahosted.org/setup/
Source0: https://fedorahosted.org/releases/s/e/%{name}/%{name}-0.1.tar.bz2
BuildArch: noarch
BuildRequires: bash
Requires: filesystem

%description
The setup package contains a set of important system configuration and
setup files, such as profile.

%prep
%setup -q

%build

%check
# Run any sanity checks.
#make check

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/etc/profile.d
cp -ar * %{buildroot}/etc
rm -f %{buildroot}/etc/uidgid
rm -f %{buildroot}/etc/COPYING
mkdir -p %{buildroot}/var/log
touch %{buildroot}/var/log/lastlog
touch %{buildroot}/etc/environment
chmod 0644 %{buildroot}/etc/environment
chmod 0644 %{buildroot}/var/log/lastlog
touch %{buildroot}/etc/fstab
touch %{buildroot}/etc/mtab

# remove unpackaged files from the buildroot
rm -f %{buildroot}/etc/Makefile
rm -f %{buildroot}/etc/serviceslint
rm -f %{buildroot}/etc/uidgidlint
rm -f %{buildroot}/etc/setup.spec
rm -rf %{buildroot}/etc/packaging

mkdir -p $RPM_BUILD_ROOT%{_datadir}/license
cat COPYING > $RPM_BUILD_ROOT%{_datadir}/license/setup

%clean
rm -rf %{buildroot}

#throw away useless and dangerous update stuff until rpm will be able to
#handle it ( http://rpm.org/ticket/6 )
#%post -p <lua>
#for i, name in ipairs({"passwd", "shadow", "group", "gshadow"}) do
#     os.remove("/etc/"..name..".rpmnew")
#end

%files
%defattr(-,root,root,-)
%{_datadir}/license/setup
%verify(not md5 size mtime) %config(noreplace) /etc/services
%verify(not md5 size mtime) %config(noreplace) /etc/exports
%config(noreplace) /etc/aliases
%config(noreplace) /etc/environment
%config(noreplace) /etc/filesystems
%config(noreplace) /etc/host.conf
%verify(not md5 size mtime) %config(noreplace) /etc/hosts
%verify(not md5 size mtime) %config(noreplace) /etc/hosts.allow
%verify(not md5 size mtime) %config(noreplace) /etc/hosts.deny
%verify(not md5 size mtime) %config(noreplace) /etc/motd
%config(noreplace) /etc/printcap
%verify(not md5 size mtime) %config(noreplace) /etc/inputrc
%config(noreplace) /etc/bashrc
%config(noreplace) /etc/profile
%verify(not md5 size mtime) %config(noreplace) /etc/protocols
%attr(0600,root,root) %config(noreplace,missingok) /etc/securetty
%config(noreplace) /etc/csh.login
%config(noreplace) /etc/csh.cshrc
%dir /etc/profile.d
%config(noreplace) %verify(not md5 size mtime) /etc/shells
%ghost %attr(0644,root,root) %verify(not md5 size mtime) /var/log/lastlog
%ghost %verify(not md5 size mtime) %config(noreplace,missingok) /etc/fstab
%ghost %verify(not md5 size mtime) %config(noreplace,missingok) /etc/mtab
