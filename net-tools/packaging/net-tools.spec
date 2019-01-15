#specfile originally created for Fedora, modified for Moblin Linux
%define npversion	1.2.9

Summary: Basic networking tools
Name: net-tools
Version: 1.60
Release: %{?release_prefix:%{release_prefix}.}87.45.%{?dist}%{!?dist:tizen}
VCS:     external/net-tools#Z910F_PROTEX_0512-2-gcf2fb2f802811f380c77fdb0b636d08f1ac1be09
License: GPL+
Group: System/Base
URL: http://www.tazenda.demon.co.uk/phil/net-tools/
Source0: http://www.tazenda.demon.co.uk/phil/net-tools/net-tools-%{version}.tar.bz2
Source1: http://www.red-bean.com/~bos/netplug/netplug-%{npversion}.tar.bz2
Source2: net-tools-%{version}-config.h
Source3: net-tools-%{version}-config.make
Source4: ether-wake.c
Source5: ether-wake.8
Source6: mii-diag.c
Source7: mii-diag.8
Source1001: %{name}.manifest
Patch1: net-tools-1.57-bug22040.patch
Patch2: net-tools-1.60-miiioctl.patch
Patch3: net-tools-1.60-manydevs.patch
Patch4: net-tools-1.60-virtualname.patch
Patch5: net-tools-1.60-cycle.patch
Patch6: net-tools-1.60-nameif.patch
Patch7: net-tools-1.60-ipx.patch
Patch8: net-tools-1.60-inet6-lookup.patch
Patch9: net-tools-1.60-man.patch
Patch10: net-tools-1.60-gcc33.patch
Patch11: net-tools-1.60-trailingblank.patch
Patch12: net-tools-1.60-interface.patch
Patch14: net-tools-1.60-gcc34.patch
Patch15: net-tools-1.60-overflow.patch
Patch19: net-tools-1.60-siunits.patch
Patch20: net-tools-1.60-trunc.patch
Patch21: net-tools-1.60-return.patch
Patch22: net-tools-1.60-parse.patch
Patch23: net-tools-1.60-netmask.patch
Patch24: net-tools-1.60-ulong.patch
Patch25: net-tools-1.60-bcast.patch
Patch26: net-tools-1.60-mii-tool-obsolete.patch
Patch27: net-tools-1.60-netstat_ulong.patch
Patch28: net-tools-1.60-note.patch
Patch29: net-tools-1.60-num-ports.patch
Patch30: net-tools-1.60-duplicate-tcp.patch
Patch31: net-tools-1.60-statalias.patch
Patch32: net-tools-1.60-isofix.patch
Patch33: net-tools-1.60-bitkeeper.patch
Patch34: net-tools-1.60-ifconfig_ib.patch
Patch35: net-tools-1.60-de.patch
Patch36: netplug-1.2.9-execshield.patch
Patch37: net-tools-1.60-pie.patch
Patch38: net-tools-1.60-ifaceopt.patch
Patch39: net-tools-1.60-trim_iface.patch
Patch40: net-tools-1.60-stdo.patch
Patch41: net-tools-1.60-statistics.patch
Patch42: net-tools-1.60-ifconfig.patch
Patch43: net-tools-1.60-arp_overflow.patch
Patch44: net-tools-1.60-hostname_man.patch
Patch45: net-tools-1.60-interface_stack.patch
Patch46: net-tools-1.60-selinux.patch
Patch47: net-tools-1.60-netstat_stop_trim.patch
Patch48: net-tools-1.60-netstat_inode.patch
Patch49: net-tools-1.60-fgets.patch
Patch50: net-tools-1.60-ifconfig_man.patch
Patch51: net-tools-1.60-x25-proc.patch
Patch52: net-tools-1.60-sctp.patch
Patch53: net-tools-1.60-arp_man.patch
Patch54: net-tools-1.60-ifconfig-long-iface-crasher.patch
Patch55: net-tools-1.60-netdevice.patch
Patch56: net-tools-1.60-skip.patch
Patch57: net-tools-1.60-netstat-I-fix.patch
Patch58: net-tools-1.60-nameif_strncpy.patch
Patch59: net-tools-1.60-arp-unaligned-access.patch
Patch60: net-tools-1.60-sctp-quiet.patch
Patch61: net-tools-1.60-remove_node.patch
Patch62: net-tools-1.60-netstat-interfaces-crash.patch
Patch63: net-tools-1.60-netplugd_init.patch
Patch64: net-tools-1.60-ec_hw_null.patch
Patch65: net-tools-1.60-statistics_buffer.patch
Patch66: net-tools-1.60-sctp-addrs.patch
Patch67: net-tools-1.60-i-option.patch
Patch68: net-tools-1.60-a-option.patch
Patch69: net-tools-1.60-clear-flag.patch
Patch70: net-tools-1.60-metric-tunnel-man.patch
Patch71: net-tools-1.60-netstat-probe.patch
Patch72: net-tools-1.60-ip.patch

BuildRequires: gettext-tools

%description
The net-tools package contains basic networking tools, including
ifconfig, netstat, route, and others.

%package extra
Summary: Extra goodies from net-tools package
Group: System/Base

%description extra
net-tools extra goodies, including not-so commonly needed tools
(netplugd, nisdomainname, ether-wake, ipmaddr, mii-diag and mii-tool,
plipconfig and slattach), translations of the man pages and
localized support.

%prep
%setup -q -a 1
%patch1 -p1 -b .bug22040
%patch2 -p1 -b .miiioctl
%patch3 -p0 -b .manydevs
%patch4 -p1 -b .virtualname
%patch5 -p1 -b .cycle
%patch6 -p1 -b .nameif
%patch7 -p1 -b .ipx
%patch8 -p1 -b .inet6-lookup
%patch9 -p1 -b .man
%patch10 -p1 -b .gcc33
%patch11 -p1 -b .trailingblank
%patch12 -p1 -b .interface
%patch14 -p1 -b .gcc34
%patch15 -p1 -b .overflow
%patch19 -p1 -b .siunits
%patch20 -p1 -b .trunc
%patch21 -p1 -b .return
%patch22 -p1 -b .parse
%patch23 -p1 -b .netmask
%patch24 -p1 -b .ulong
%patch25 -p1 -b .bcast
%patch26 -p1 -b .obsolete
%patch27 -p1 -b .netstat_ulong
%patch28 -p1 -b .note
%patch29 -p1 -b .num-ports
%patch30 -p1 -b .dup-tcp
%patch31 -p1 -b .statalias
%patch32 -p1 -b .isofix
%patch33 -p1 -b .bitkeeper
%patch34 -p1 -b .ifconfig_ib
%patch35 -p1
%patch36 -p1 -b .execshield
%patch37 -p1 -b .pie
%patch38 -p1 -b .ifaceopt
%patch39 -p1 -b .trim-iface
%patch40 -p1 -b .stdo
%patch41 -p1 -b .statistics
%patch42 -p1 -b .iface_drop
%patch43 -p1 -b .overflow
%patch44 -p1 -b .hostname_man
%patch45 -p0 -b .stack
%patch46 -p1 -b .selinux
%patch47 -p1 -b .trim
%patch48 -p1 -b .inode
%patch49 -p1 -b .fgets
%patch50 -p1 -b .inet_addr
%patch51 -p1 -b .x25
%patch52 -p1 -b .sctp
%patch53 -p1
%patch54 -p1 -b .long_iface
%patch55 -p1 -b .netdevice
%patch56 -p1 -b .skip
%patch57 -p1
%patch58 -p1 -b .strncpy
%patch59 -p1 -b .arp-un-access
%patch60 -p1 -b .quiet
%patch61 -p1
%patch62 -p1 -b .iface-crash
%patch63 -p1
%patch64 -p1
%patch65 -p1 -b .buffer
%patch66 -p1 -b .sctp-addrs
%patch67 -p1 -b .i-option
%patch68 -p1 -b .a-option
%patch69 -p1 -b .clear-flag
%patch70 -p1 -b .metric-tunnel-man
%patch71 -p1 -b .probe

# after splitting the kernel header package, this is not needed anymore?
%patch72 -p1 -b .iptunnel


cp %SOURCE2 ./config.h
cp %SOURCE3 ./config.make
cp %SOURCE4 .
cp %SOURCE5 ./man/en_US
cp %SOURCE6 .
cp %SOURCE7 ./man/en_US



%build
cp %{SOURCE1001} .
sed -i "s/HAVE_SELINUX=1/HAVE_SELINUX=0/g" ./config.make
make
gcc $RPM_OPT_FLAGS -o ether-wake ether-wake.c
gcc $RPM_OPT_FLAGS -o mii-diag mii-diag.c

%install

make BASEDIR=$RPM_BUILD_ROOT mandir=%{_mandir} install

install -m 755 ether-wake %{buildroot}/sbin
install -m 755 mii-diag %{buildroot}/sbin


rm %{buildroot}/sbin/rarp
rm -rf %{buildroot}%{_mandir}/*/man*


%find_lang %{name}

mkdir -p $RPM_BUILD_ROOT%{_datadir}/license
for keyword in LICENSE COPYING COPYRIGHT;
do
	for file in `find %{_builddir} -name $keyword`;
	do
		cat $file >> $RPM_BUILD_ROOT%{_datadir}/license/%{name};
		echo "";
	done;
done

%docs_package

%lang_package

%files
%defattr(-,root,root,-)
%manifest %{name}.manifest
%{_datadir}/license/%{name}
/bin/*
/sbin/*
%exclude /bin/nisdomainname
%exclude /bin/ypdomainname
%exclude /sbin/ether-wake
%exclude /sbin/ipmaddr
%exclude /sbin/mii-diag
%exclude /sbin/mii-tool
%exclude /sbin/plipconfig
%exclude /sbin/slattach
%doc COPYING

%files extra
/bin/nisdomainname
/bin/ypdomainname
/sbin/ether-wake
/sbin/ipmaddr
/sbin/mii-diag
/sbin/mii-tool
/sbin/plipconfig
/sbin/slattach
%changelog
* Tue Jun 17 2014 WaLyong Cho <walyong.cho@samsung.com> - None 
- PROJECT: external/net-tools
- COMMIT_ID: cf2fb2f802811f380c77fdb0b636d08f1ac1be09
- BRANCH: master
- PATCHSET_REVISION: cf2fb2f802811f380c77fdb0b636d08f1ac1be09
- CHANGE_OWNER: \"WaLyong Cho\" <walyong.cho@samsung.com>
- PATCHSET_UPLOADER: \"WaLyong Cho\" <walyong.cho@samsung.com>
- CHANGE_URL: http://slp-info.sec.samsung.net/gerrit/519585
- PATCHSET_REVISION: cf2fb2f802811f380c77fdb0b636d08f1ac1be09
- TAGGER: WaLyong Cho <walyong.cho@samsung.com>
- Gerrit patchset approval info:
- WaLyong Cho <walyong.cho@samsung.com> Verified : 1
- Kim ByungSoo <bs1770.kim@samsung.com> Code-Review : 2
- CHANGE_SUBJECT: Merge branch 'devel/systemfw/master'
- Merge branch 'devel/systemfw/master'
* Wed Nov 27 2013 WaLyong Cho <walyong.cho@samsung.com> - None 
- PROJECT: external/net-tools
- COMMIT_ID: 3c2a1661fcff16ee887977b16560303e141ba775
- PATCHSET_REVISION: 3c2a1661fcff16ee887977b16560303e141ba775
- CHANGE_OWNER: \"WaLyong Cho\" <walyong.cho@samsung.com>
- PATCHSET_UPLOADER: \"WaLyong Cho\" <walyong.cho@samsung.com>
- CHANGE_URL: http://slp-info.sec.samsung.net/gerrit/348433
- PATCHSET_REVISION: 3c2a1661fcff16ee887977b16560303e141ba775
- TAGGER: WaLyong Cho <walyong.cho@samsung.com>
- Gerrit patchset approval info:
- Kyungmin Park <kyungmin.park@samsung.com> Code Review : 2
- Kyungmin Park <kyungmin.park@samsung.com> Verified : 1
- WaLyong Cho <walyong.cho@samsung.com> Verified : 1
- CHANGE_SUBJECT: Merge branch 'devel/systemfw/master'
- Merge branch 'devel/systemfw/master'
* Mon Sep 16 2013 walyong.cho <walyong.cho@samsung.com> - net-tools_1.60-slp2+s87.3_1 
- PROJECT: external/net-tools
- COMMIT_ID: 5eeb194f7720cff1d8cb8f939d1f170fb6626c89
- PATCHSET_REVISION: 5eeb194f7720cff1d8cb8f939d1f170fb6626c89
- CHANGE_OWNER: \"WaLyong Cho\" <walyong.cho@samsung.com>
- PATCHSET_UPLOADER: \"WaLyong Cho\" <walyong.cho@samsung.com>
- CHANGE_URL: http://slp-info.sec.samsung.net/gerrit/147777
- PATCHSET_REVISION: 5eeb194f7720cff1d8cb8f939d1f170fb6626c89
- TAGGER: walyong.cho <walyong.cho@samsung.com>
- Gerrit patchset approval info:
- Kyungmin Park <kyungmin.park@samsung.com> Code Review : 2
- Kyungmin Park <kyungmin.park@samsung.com> Verified : 1
- CHANGE_SUBJECT: smack execution label delete
- [Version] net-tools 1.60-87.3
- [Project] GT-I8800, Public
- [Title] smack execution label delete
- [BinType] PDA/MODEM/CSC
- [Issue#] N/A
- [Problem] smack execution label delete
- [Cause] smack execution label delete
- [Solution] smack execution label delete
- [Team] System FW
- [Developer]  walyong.cho
- [Request] N/A
- [Horizontal expansion] N/A
- [SCMRequest] N/A
* Mon Feb  4 2013 walyong.cho <walyong.cho@samsung.com> - net-tools_1.60-slp2+s87.3_1 
- [Version] net-tools 1.60-87.3
- [Project] GT-I8800, Public
- [Title] smack execution label delete
- [BinType] PDA/MODEM/CSC
- [Issue#] N/A
- [Problem] smack execution label delete
- [Cause] smack execution label delete
- [Solution] smack execution label delete
- [Team] System FW
- [Developer]  walyong.cho
- [Request] N/A
- [Horizontal expansion] N/A
- [SCMRequest] N/A
