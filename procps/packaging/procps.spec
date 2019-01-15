Summary: System and process monitoring utilities
Name: procps
Version: 3.2.8
Release: 17
License: GPLv2+ and LGPLv2+
Group: Applications/System
URL: http://procps.sourceforge.net
Source: http://procps.sourceforge.net/procps-%{version}.tar.gz
Source1: FAQ
Source1001: %{name}.manifest
Buildroot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

Patch1: procps-3.2.8-selinux.patch
Patch2: procps-3.2.7-misc.patch
Patch3: procps-3.2.7-FAQ.patch
Patch6: procps-3.2.7-noproc.patch
Patch7: procps-3.2.7-pseudo.patch
Patch8: procps-3.2.7-0x9b.patch
# 157725 - sysctl -A returns an error
Patch9: procps-3.2.7-sysctl-writeonly.patch
# 161449 - "top" ignores user and system toprc
Patch10: procps-3.2.7-top-rc.patch
# 161303 - 'top' failed when remove cpus
# 186017 - Top "Cpu0" line never updates on single processor machine
Patch11: procps-3.2.7-top-remcpu.patch
# 177453 - for VIRT use proc->vm_size rather then proc->size (workaround)
Patch14: procps-3.2.7-top-env-vmsize.patch
# 174619 - workaround for reliable Cpu(s) data in the first loop
Patch15: procps-3.2.7-top-env-cpuloop.patch
# 185299 - cpu steal time support
Patch16: procps-3.2.7-vmstat-cpusteal.patch
# 134516 - ps ignores /proc/#/cmdline if contents 2047 bytes
Patch17: procps-3.2.7-longcmd.patch
# 189349 - 32bit vmstat on 64bit kernel
Patch18: procps-3.2.7-vmstat-pgpg.patch
# 212637 - sysctl using deprecated syscall
# 228870 - process `sysctl' is using deprecated sysctl ...
Patch21: procps-3.2.7-sysctl-ignore.patch
# 140975 - top corrupts screen when sorting on first column
Patch22: procps-3.2.7-top-sorthigh.path
# 234546 - 'w' doesn't give correct information about what's being run.
Patch23: procps-3.2.7-w-best.patch
# 183029 - watch ignores unicode characters
#Patch24: procps-3.2.7-watch-unicode.patch
# 222251 - STIME can jitter by one second
Patch26: procps-3.2.7-ps-stime.patch
#244152 - ps truncates eip and esp to 32-bit values on 64-bit systems
Patch28: procps-3.2.7-ps-eip64.patch
#244960 - ps manpage formatted incorrectly
Patch29: procps-3.2.7-psman.patch
#185994 - error when using "Single Cpu = Off" option
Patch31: procps-3.2.7-top-cpu0.patch
#354001 - CPU value in top is reported as an integer
Patch32: procps-3.2.7-top-cpuint.patch
#296471 - update top man page
Patch33: procps-3.2.7-top-manpage.patch
#440694 - strange text after selecting few field
Patch34: procps-3.2.7-top-clrscr.patch
#435453 - errors with man -t formatting of ps man page
Patch35: procps-3.2.7-ps-man-fmt.patch
#472783 - 'vmstat -p <partition name>',
# the detailed statistics of the partition name is not output.
Patch36: procps-3.2.7-vmstat-partstats-long.patch
# Fix vmstat header to be 80 chars not 81
Patch37: procps-3.2.7-vmstat-header.patch
# rhel bug #475963: slabtop -o should display the info once
Patch38: procps-3.2.7-slabtop-once.patch
#476134 - added timestamp to vmstat with new option -t
Patch39: procps-3.2.7-vmstat-timestamp.patch
#manual page updated to document the -t functionality
Patch40: procps-3.2.7-vmstat-timestamp-manpage.patch
#added cgroup display to ps
Patch41: procps-3.2.7-ps-cgroup.patch
# 'requested writes' display in partition statistics
Patch42: procps-3.2.7-vmstat-partstats-reqwrites.patch
# '-l' option of 'free' documented
Patch43: procps-3.2.7-free-hlmem.patch
# bug in showing threads fixed
Patch44: procps-3.2.8-threads.patch
# enable core file generation (don't trap it)
Patch45: procps-enable-core.patch
#548711 -  [abrt] crash in procps-3.2.8-3.fc12
Patch46: procps-3.2.8-setlocale.patch
# debian bug #505571 pmap -x should show more information
Patch47: procps-pmap-smaps.patch
#578798 - vmstat -SM doesn't work but vmstat -S M does
#Patch48: procps-3.2.7-vmstat-sm.patch
Patch48: procps-3.2.8-vmstat-getopt.patch
Patch49: procps-3.2.8-ps-cgroup-suppress-root-group.patch
#631340 - fixed build with make 3.82
Patch50: procps-3.2.8-make.patch
#632236: don't depend on constructor execution order
Patch51: procps-3.2.8-constructor-order.patch

BuildRequires: ncurses-devel

%description
The procps package contains a set of system utilities that provide
system information. Procps includes ps, free, skill, pkill, pgrep,
snice, tload, top, uptime, vmstat, w, watch and pdwx. The ps command
displays a snapshot of running processes. The top command provides
a repetitive update of the statuses of running processes. The free
command displays the amounts of free and used memory on your
system. The skill command sends a terminate command (or another
specified signal) to a specified set of processes. The snice
command is used to change the scheduling priority of specified
processes. The tload command prints a graph of the current system
load average to a specified tty. The uptime command displays the
current time, how long the system has been running, how many users
are logged on, and system load averages for the past one, five,
and fifteen minutes. The w command displays a list of the users
who are currently logged on and what they are running. The watch
program watches a running program. The vmstat command displays
virtual memory statistics about processes, memory, paging, block
I/O, traps, and CPU activity. The pwdx command reports the current
working directory of a process or processes.

%package devel
Summary:  System and process monitoring utilities
Group:    Development/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
System and process monitoring utilities development headers

%prep
%setup -q

%patch1 -p1
%patch2 -p1
%patch3 -p1
%patch6 -p1
%patch7 -p1
%patch8 -p1
%patch9 -p1
%patch10 -p1
%patch11 -p1
%patch14 -p1
%patch15 -p1
%patch16 -p1
%patch17 -p1
%patch18 -p1
%patch21 -p1
%patch22 -p1
%patch23 -p1
#%patch24 -p1
%patch26 -p1
%patch28 -p1
%patch29 -p1
%patch31 -p1
%patch32 -p1
%patch33 -p1
%patch34 -p1
%patch35 -p1
%patch36 -p1
%patch37 -p1
%patch38 -p1
%patch39 -p1
%patch40 -p1
%patch41 -p1
%patch42 -p1
%patch43 -p1
%patch44 -p1
%patch45 -p1
%patch46 -p1
%patch47 -p1
%patch48 -p1
%patch49 -p1
%patch50 -p1
%patch51 -p1

cp %SOURCE1 .

%build
cp %{SOURCE1001} .
export CFLAGS="$RPM_OPT_FLAGS"
%ifnarch x86_64
export CFLAGS+=" -fpie"
export LDFLAGS="-pie"
%endif
make %{?_smp_mflags} CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" SHARED=1 W_SHOWFROM=-DW_SHOWFROM lib64=%{_lib}

%install
rm -rf %{buildroot}
make ldconfig=true DESTDIR=%{buildroot} lib64=%{_lib} install="install -D" \
      SKIP="/bin/kill /usr/share/man/man1/kill.1" install
mkdir -p %{buildroot}/%{_docdir}/procps-%{version}
# keep 'rpm' happy...
chmod -R u+w %{buildroot}/sbin
chmod -R u+w %{buildroot}/bin
chmod -R u+w %{buildroot}/usr/bin
chmod -R u+w %{buildroot}/lib*

mkdir -p %{buildroot}/%{_includedir}/proc
install -Dpm 644 proc/*.h %{buildroot}/%{_includedir}/proc
pushd %{buildroot}/%{_lib}
ln -s libproc-%{version}.so libproc.so
popd

mkdir -p $RPM_BUILD_ROOT%{_datadir}/license
for keyword in LICENSE COPYING COPYRIGHT;
do
	for file in `find %{_builddir} -name $keyword`;
	do
		cat $file >> $RPM_BUILD_ROOT%{_datadir}/license/%{name};
		echo "";
	done;
done

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(0644,root,root,755)
%manifest %{name}.manifest
%doc NEWS BUGS TODO FAQ
%{_datadir}/license/%{name}
%attr(755,root,root) /%{_lib}/libproc-%{version}.so
%attr(755,root,root) /bin/ps
%attr(755,root,root) /sbin/sysctl
%attr(755,root,root) /usr/bin/*

%attr(0644,root,root) %{_mandir}/man1/*
%attr(0644,root,root) %{_mandir}/man8/*
%attr(0644,root,root) %{_mandir}/man5/*

%files devel
%defattr(-,root,root,-)
%{_includedir}/proc
/%{_lib}/libproc.so
