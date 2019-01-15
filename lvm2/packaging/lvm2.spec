%define device_mapper_version 1.02.99

Summary: Userland logical volume management tools 
Name: lvm2
Version: 2.02.122
Release: 1
License: GPLv2
Group: System Environment/Base
URL: http://sources.redhat.com/lvm2
Source0: ftp://sources.redhat.com/pub/lvm2/releases/LVM2.%{version}.tgz

#BuildRequires: libselinux-devel >= %{libselinux_version}, libsepol-devel
BuildRequires: libblkid-devel >= %{util_linux_version}
BuildRequires: ncurses-devel
BuildRequires: readline-devel
BuildRequires: module-init-tools
BuildRequires: pkgconfig
BuildRequires: systemd-devel
BuildRequires: systemd-units
Requires: %{name}-libs = %{version}-%{release}
Requires: bash >= %{bash_version}
Requires(post): systemd-units >= %{systemd_version}
Requires(preun): systemd-units >= %{systemd_version}
Requires(postun): systemd-units >= %{systemd_version}
Requires: module-init-tools

%description
LVM2 includes all of the support for handling read/write operations on
physical volumes (hard disks, RAID-Systems, magneto optical, etc.,
multiple devices (MD), see mdadd(8) or even loop devices, see
losetup(8)), creating volume groups (kind of virtual disks) from one
or more physical volumes and creating one or more logical volumes
(kind of logical partitions) in volume groups.

%prep
%setup -q -n LVM2.%{version}

%build
%configure \
        --prefix=/usr \
        --disable-selinux \

make %{?_smp_mflags}

%install
make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig

%preun

%postun

%files
%defattr(-,root,root,-)
%{!?_licensedir:%global license %%doc}
%license COPYING COPYING.LIB
%doc README VERSION WHATS_NEW
%doc doc/lvm_fault_handling.txt

# Main binaries
%defattr(555,root,root,-)
%{_sbindir}/fsadm
%{_sbindir}/lvm
%{_sbindir}/lvmconf
%{_sbindir}/lvmconfig
%{_sbindir}/lvmdump
%{_sbindir}/vgimportclone

# Other files
%defattr(444,root,root,-)
%{_sbindir}/lvchange
%{_sbindir}/lvconvert
%{_sbindir}/lvcreate
%{_sbindir}/lvdisplay
%{_sbindir}/lvextend
%{_sbindir}/lvmchange
%{_sbindir}/lvmdiskscan
%{_sbindir}/lvmsadc
%{_sbindir}/lvmsar
%{_sbindir}/lvreduce
%{_sbindir}/lvremove
%{_sbindir}/lvrename
%{_sbindir}/lvresize
%{_sbindir}/lvs
%{_sbindir}/lvscan
%{_sbindir}/pvchange
%{_sbindir}/pvck
%{_sbindir}/pvcreate
%{_sbindir}/pvdisplay
%{_sbindir}/pvmove
%{_sbindir}/pvremove
%{_sbindir}/pvresize
%{_sbindir}/pvs
%{_sbindir}/pvscan
%{_sbindir}/vgcfgbackup
%{_sbindir}/vgcfgrestore
%{_sbindir}/vgchange
%{_sbindir}/vgck
%{_sbindir}/vgconvert
%{_sbindir}/vgcreate
%{_sbindir}/vgdisplay
%{_sbindir}/vgexport
%{_sbindir}/vgextend
%{_sbindir}/vgimport
%{_sbindir}/vgmerge
%{_sbindir}/vgmknodes
%{_sbindir}/vgreduce
%{_sbindir}/vgremove
%{_sbindir}/vgrename
%{_sbindir}/vgs
%{_sbindir}/vgscan
%{_sbindir}/vgsplit
%{_mandir}/man5/lvm.conf.5.gz
%{_mandir}/man7/lvmcache.7.gz
%{_mandir}/man7/lvmthin.7.gz
%{_mandir}/man7/lvmsystemid.7.gz
%{_mandir}/man8/fsadm.8.gz
%{_mandir}/man8/lvchange.8.gz
%{_mandir}/man8/lvconvert.8.gz
%{_mandir}/man8/lvcreate.8.gz
%{_mandir}/man8/lvdisplay.8.gz
%{_mandir}/man8/lvextend.8.gz
%{_mandir}/man8/lvm.8.gz
%{_mandir}/man8/lvm-lvpoll.8.gz
# %{_mandir}/man8/lvm2-activation-generator.8.gz
%{_mandir}/man8/lvm-config.8.gz
%{_mandir}/man8/lvmconfig.8.gz
%{_mandir}/man8/lvm-dumpconfig.8.gz
%{_mandir}/man8/lvmchange.8.gz
%{_mandir}/man8/lvmconf.8.gz
%{_mandir}/man8/lvmdiskscan.8.gz
%{_mandir}/man8/lvmdump.8.gz
%{_mandir}/man8/lvmsadc.8.gz
%{_mandir}/man8/lvmsar.8.gz
%{_mandir}/man8/lvreduce.8.gz
%{_mandir}/man8/lvremove.8.gz
%{_mandir}/man8/lvrename.8.gz
%{_mandir}/man8/lvresize.8.gz
%{_mandir}/man8/lvs.8.gz
%{_mandir}/man8/lvscan.8.gz
%{_mandir}/man8/pvchange.8.gz
%{_mandir}/man8/pvck.8.gz
%{_mandir}/man8/pvcreate.8.gz
%{_mandir}/man8/pvdisplay.8.gz
%{_mandir}/man8/pvmove.8.gz
%{_mandir}/man8/pvremove.8.gz
%{_mandir}/man8/pvresize.8.gz
%{_mandir}/man8/pvs.8.gz
%{_mandir}/man8/pvscan.8.gz
%{_mandir}/man8/vgcfgbackup.8.gz
%{_mandir}/man8/vgcfgrestore.8.gz
%{_mandir}/man8/vgchange.8.gz
%{_mandir}/man8/vgck.8.gz
%{_mandir}/man8/vgconvert.8.gz
%{_mandir}/man8/vgcreate.8.gz
%{_mandir}/man8/vgdisplay.8.gz
%{_mandir}/man8/vgexport.8.gz
%{_mandir}/man8/vgextend.8.gz
%{_mandir}/man8/vgimport.8.gz
%{_mandir}/man8/vgimportclone.8.gz
%{_mandir}/man8/vgmerge.8.gz
%{_mandir}/man8/vgmknodes.8.gz
%{_mandir}/man8/vgreduce.8.gz
%{_mandir}/man8/vgremove.8.gz
%{_mandir}/man8/vgrename.8.gz
%{_mandir}/man8/vgs.8.gz
%{_mandir}/man8/vgscan.8.gz
%{_mandir}/man8/vgsplit.8.gz
# %{_libdir}/udev/rules.d/11-dm-lvm.rules
%dir %{_sysconfdir}/lvm
%attr(644, -, -) %ghost %{_sysconfdir}/lvm/cache/.cache
%attr(644, -, -) %config(noreplace) %verify(not md5 mtime size) %{_sysconfdir}/lvm/lvm.conf
%attr(644, -, -) %config(noreplace) %verify(not md5 mtime size) %{_sysconfdir}/lvm/lvmlocal.conf
%dir %{_sysconfdir}/lvm/profile
%{_sysconfdir}/lvm/profile/command_profile_template.profile
%{_sysconfdir}/lvm/profile/metadata_profile_template.profile
%{_sysconfdir}/lvm/profile/thin-generic.profile
%{_sysconfdir}/lvm/profile/thin-performance.profile
# %dir %{_sysconfdir}/lvm/backup
# %dir %{_sysconfdir}/lvm/cache
# %dir %{_sysconfdir}/lvm/archive
%ghost %dir %attr(755, -, -)  /run/lock/lvm
%ghost %dir %attr(755, -, -)  /run/lvm
# %{_tmpfilesdir}/%{name}.conf
# %{_unitdir}/blk-availability.service
# %{_unitdir}/lvm2-monitor.service
# %attr(555, -, -) %{_prefix}/lib/systemd/system-generators/lvm2-activation-generator

##############################################################################
# Library and Development subpackages
##############################################################################
%package devel
Summary: Development libraries and headers
Group: Development/Libraries
License: LGPLv2
Requires: %{name} = %{version}-%{release}
Requires: device-mapper-devel = %{device_mapper_version}-%{release}
Requires: device-mapper-event-devel = %{device_mapper_version}-%{release}
Requires: pkgconfig

%description devel
This package contains files needed to develop applications that use
the lvm2 libraries.

%files devel
%defattr(444,root,root,-)
# %{_libdir}/liblvm2app.so
# %{_libdir}/liblvm2cmd.so
# %{_libdir}/libdevmapper-event-lvm2.so
# %{_includedir}/lvm2app.h
# %{_includedir}/lvm2cmd.h
# %{_libdir}/pkgconfig/lvm2app.pc

%package libs
Summary: Shared libraries for lvm2
License: LGPLv2
Group: System Environment/Libraries
Requires: device-mapper-event = %{device_mapper_version}-%{release}

%description libs
This package contains shared lvm2 libraries for applications.

%post libs -p /sbin/ldconfig

%postun libs -p /sbin/ldconfig

%files libs
%defattr(555,root,root,-)
%{!?_licensedir:%global license %%doc}
%license COPYING.LIB
# %{_libdir}/liblvm2app.so.*
# %{_libdir}/liblvm2cmd.so.*
# %{_libdir}/libdevmapper-event-lvm2.so.*
# %dir %{_libdir}/device-mapper
# %{_libdir}/device-mapper/libdevmapper-event-lvm2mirror.so
# %{_libdir}/device-mapper/libdevmapper-event-lvm2snapshot.so
# %{_libdir}/device-mapper/libdevmapper-event-lvm2raid.so
# %{_libdir}/libdevmapper-event-lvm2mirror.so
# %{_libdir}/libdevmapper-event-lvm2snapshot.so
# %{_libdir}/libdevmapper-event-lvm2raid.so

##############################################################################
# Device-mapper subpackages
##############################################################################
%package -n device-mapper
Summary: Device mapper utility
Version: %{device_mapper_version}
License: GPLv2
Group: System Environment/Base
URL: http://sources.redhat.com/dm
Requires: device-mapper-libs = %{device_mapper_version}-%{release}
Requires: util-linux >= %{util_linux_version}
Requires: systemd >= %{systemd_version}
# We need dracut to install required udev rules if udev_sync
# feature is turned on so we don't lose required notifications.
Conflicts: dracut < %{dracut_version}

%description -n device-mapper
This package contains the supporting userspace utility, dmsetup,
for the kernel device-mapper.

%files -n device-mapper
%defattr(-,root,root,-)
%{!?_licensedir:%global license %%doc}
%license COPYING COPYING.LIB
%doc WHATS_NEW_DM VERSION_DM README
%doc udev/12-dm-permissions.rules
%defattr(444,root,root,-)
%attr(555, -, -) %{_sbindir}/dmsetup
%attr(555, -, -) %{_sbindir}/blkdeactivate
%{_mandir}/man8/dmsetup.8.gz
%{_mandir}/man8/blkdeactivate.8.gz
# %{_libdir}/udev/rules.d/10-dm.rules
# %{_libdir}/udev/rules.d/13-dm-disk.rules
# %{_libdir}/udev/rules.d/95-dm-notify.rules

%package -n device-mapper-devel
Summary: Development libraries and headers for device-mapper
Version: %{device_mapper_version}
License: LGPLv2
Group: Development/Libraries
Requires: device-mapper = %{device_mapper_version}-%{release}
Requires: pkgconfig

%description -n device-mapper-devel
This package contains files needed to develop applications that use
the device-mapper libraries.

%files -n device-mapper-devel
%defattr(444,root,root,-)
%{_libdir}/libdevmapper.so
%{_includedir}/libdevmapper.h
# %{_libdir}/pkgconfig/devmapper.pc

%package -n device-mapper-libs
Summary: Device-mapper shared library
Version: %{device_mapper_version}
License: LGPLv2
Group: System Environment/Libraries
Requires: device-mapper = %{device_mapper_version}-%{release}

%description -n device-mapper-libs
This package contains the device-mapper shared library, libdevmapper.

%post -n device-mapper-libs -p /sbin/ldconfig

%postun -n device-mapper-libs -p /sbin/ldconfig

%files -n device-mapper-libs
%defattr(555,root,root,-)
%{!?_licensedir:%global license %%doc}
%license COPYING COPYING.LIB
%{_libdir}/libdevmapper.so.*

%package -n device-mapper-event
Summary: Device-mapper event daemon
Group: System Environment/Base
Version: %{device_mapper_version}
Requires: device-mapper = %{device_mapper_version}-%{release}
Requires: device-mapper-event-libs = %{device_mapper_version}-%{release}
Requires(post): systemd-units
Requires(preun): systemd-units
Requires(postun): systemd-units

%description -n device-mapper-event
This package contains the dmeventd daemon for monitoring the state
of device-mapper devices.

%post -n device-mapper-event
%systemd_post dm-event.socket
# dm-event.socket is always enabled and started and ready to serve if dmeventd is used
# replace direct systemctl calls with systemd rpm macro once this is provided in the macro:
# http://cgit.freedesktop.org/systemd/systemd/commit/?id=57ab2eabb8f92fad5239c7d4492e9c6e23ee0678
systemctl enable dm-event.socket
systemctl start dm-event.socket
if [ -e %{_default_pid_dir}/dmeventd.pid ]; then
	%{_sbindir}/dmeventd -R || echo "Failed to restart dmeventd daemon. Please, try manual restart."
fi

%preun -n device-mapper-event
%systemd_preun dm-event.service dm-event.socket

%files -n device-mapper-event
%defattr(444,root,root,-)
# %attr(555, -, -) %{_sbindir}/dmeventd
# %{_mandir}/man8/dmeventd.8.gz
# %{_unitdir}/dm-event.socket
# %{_unitdir}/dm-event.service

%package -n device-mapper-event-libs
Summary: Device-mapper event daemon shared library
Version: %{device_mapper_version}
License: LGPLv2
Group: System Environment/Libraries

%description -n device-mapper-event-libs
This package contains the device-mapper event daemon shared library,
libdevmapper-event.

%post -n device-mapper-event-libs -p /sbin/ldconfig

%postun -n device-mapper-event-libs -p /sbin/ldconfig

%files -n device-mapper-event-libs
%defattr(555,root,root,-)
%{!?_licensedir:%global license %%doc}
%license COPYING.LIB
# %{_libdir}/libdevmapper-event.so.*

%package -n device-mapper-event-devel
Summary: Development libraries and headers for the device-mapper event daemon
Version: %{device_mapper_version}
License: LGPLv2
Group: Development/Libraries
Requires: device-mapper-event = %{device_mapper_version}-%{release}
Requires: pkgconfig

%description -n device-mapper-event-devel
This package contains files needed to develop applications that use
the device-mapper event library.

%files -n device-mapper-event-devel
%defattr(444,root,root,-)
# %{_libdir}/libdevmapper-event.so
# %{_includedir}/libdevmapper-event.h
# %{_libdir}/pkgconfig/devmapper-event.pc
