#
# Please submit bugfixes or comments via http://bugs.meego.com/
#

%define	_root_sbindir	/sbin
%define	_root_libdir	/%{_lib}

Name:           e2fsprogs
Version:        1.42.13
Release:        1
# License tags based on COPYING file distinctions for various components
License:        GPLv2
Summary:        Utilities for managing ext2, ext3, and ext4 filesystems
Group:          System/Base
Source0:        http://downloads.sourceforge.net/%{name}/%{name}-%{version}.tar.gz
Source1:        ext2_types-wrapper.h
Source1001:     e2fsprogs.manifest

Url:            http://e2fsprogs.sourceforge.net/
BuildRequires:  pkgconfig
BuildRequires:  pkgconfig(blkid)
BuildRequires:  pkgconfig(uuid)

%description
The e2fsprogs package contains a number of utilities for creating,
checking, modifying, and correcting any inconsistencies in second,
third and fourth extended (ext2/ext3/ext4) filesystems. E2fsprogs
contains e2fsck (used to repair filesystem inconsistencies after an
unclean shutdown), mke2fs (used to initialize a partition to contain
an empty ext2 filesystem), debugfs (used to examine the internal
structure of a filesystem, to manually repair a corrupted
filesystem, or to create test cases for e2fsck), tune2fs (used to
modify filesystem parameters), and most of the other core ext2fs
filesystem utilities.

You should install the e2fsprogs package if you need to manage the
performance of an ext2, ext3, or ext4 filesystem.

%package libs
License:        GPLv2 and LGPLv2
Summary:        Ext2/3/4 filesystem-specific shared libraries and headers
Group:          System/Libraries
Requires(post): /sbin/ldconfig

%description libs
E2fsprogs-libs contains libe2p and libext2fs, the libraries of the
e2fsprogs package.

These libraries are used to directly acccess ext2/3/4 filesystems
from userspace.

%package devel
License:        GPLv2 and LGPLv2
Summary:        Ext2/3/4 filesystem-specific static libraries and headers
Group:          Development/Libraries
Requires:       e2fsprogs-libs = %{version}
Requires:       gawk
Requires:       pkgconfig(com_err)

%description devel
E2fsprogs-devel contains the libraries and header files needed to
develop second, third and fourth extended (ext2/ext3/ext4)
filesystem-specific programs.

You should install e2fsprogs-devel if you want to develop ext2/3/4
filesystem-specific programs. If you install e2fsprogs-devel, you'll
also want to install e2fsprogs.

%package -n libcom_err
License:        MIT
Summary:        Common error description library
Group:          System/Libraries

%description -n libcom_err
This is the common error description library, part of e2fsprogs.

libcom_err is an attempt to present a common error-handling mechanism.

%package -n libcom_err-devel
License:        MIT
Summary:        Common error description library
Group:          Development/Libraries
Requires:       libcom_err = %{version}
Requires:       pkgconfig

%description -n libcom_err-devel
This is the common error description development library and headers,
part of e2fsprogs.  It contains the compile_et commmand, used
to convert a table listing error-code names and associated messages
messages into a C source file suitable for use with the library.

libcom_err is an attempt to present a common error-handling mechanism.

%package -n libss
License:        MIT
Summary:        Command line interface parsing library
Group:          System/Libraries

%description -n libss
This is libss, a command line interface parsing library, part of e2fsprogs.

This package includes a tool that parses a command table to generate
a simple command-line interface parser, the include files needed to
compile and use it, and the static libs.

It was originally inspired by the Multics SubSystem library.

%package -n libss-devel
License:        MIT
Summary:        Command line interface parsing library
Group:          Development/Libraries
Requires:       libss = %{version}
Requires:       pkgconfig

%description -n libss-devel
This is the command line interface parsing (libss) development library
and headers, part of e2fsprogs.  It contains the mk_cmds command, which
parses a command table to generate a simple command-line interface parser.

It was originally inspired by the Multics SubSystem library.

%prep
%setup -q -n e2fsprogs-%{version}

%build
cp %{SOURCE1001} .
%configure \
    --enable-elf-shlibs \
    --disable-nls \
    --disable-e2initrd-helper \
    --disable-defrag \
    --enable-fsck \
    --disable-libuuid \
    --disable-libblkid \
    --disable-uuidd \

make %{?_smp_mflags} V=1

%install
export PATH=/sbin:$PATH
make install install-libs DESTDIR=%{buildroot} INSTALL="install -p" \
	root_sbindir=%{_root_sbindir} root_libdir=%{_root_libdir}

# ugly hack to allow parallel install of 32-bit and 64-bit -devel packages:
%define multilib_arches %{ix86} x86_64

%ifarch %{multilib_arches}
mv -f %{buildroot}%{_includedir}/ext2fs/ext2_types.h \
      %{buildroot}%{_includedir}/ext2fs/ext2_types-%{_arch}.h
install -p -m 644 %{SOURCE1} %{buildroot}%{_includedir}/ext2fs/ext2_types.h
%endif

mkdir -p %{buildroot}%{_datadir}/license
cat COPYING > %{buildroot}%{_datadir}/license/e2fsprogs
cat COPYING > %{buildroot}%{_datadir}/license/e2fsprogs-libs
cat COPYING > %{buildroot}%{_datadir}/license/libcom_err
cat COPYING > %{buildroot}%{_datadir}/license/libss

chmod -R u+w %{buildroot}/*

%clean
rm -rf %{buildroot}

%docs_package

%files
%defattr(-,root,root,-)
%doc README
%{_datadir}/license/e2fsprogs
%config(noreplace) %{_sysconfdir}/mke2fs.conf
%{_root_sbindir}/badblocks
%{_root_sbindir}/debugfs
%{_root_sbindir}/dumpe2fs
%{_root_sbindir}/e2fsck
%{_root_sbindir}/e2image
%{_root_sbindir}/e2label
%{_root_sbindir}/e2undo
%{_root_sbindir}/fsck
%{_root_sbindir}/fsck.ext2
%{_root_sbindir}/fsck.ext3
%{_root_sbindir}/fsck.ext4
%{_root_sbindir}/fsck.ext4dev
%{_root_sbindir}/logsave
%{_root_sbindir}/mke2fs
%{_root_sbindir}/mkfs.ext2
%{_root_sbindir}/mkfs.ext3
%{_root_sbindir}/mkfs.ext4
%{_root_sbindir}/mkfs.ext4dev
%{_root_sbindir}/resize2fs
%{_root_sbindir}/tune2fs
%{_sbindir}/filefrag
%{_sbindir}/mklost+found
%{_sbindir}/e2freefrag
%{_bindir}/chattr
%{_bindir}/lsattr
%manifest e2fsprogs.manifest

%files libs
%defattr(-,root,root)
%{_datadir}/license/e2fsprogs-libs
%{_root_libdir}/libe2p.so.*
%{_root_libdir}/libext2fs.so.*
%manifest e2fsprogs.manifest

%files devel
%defattr(-,root,root)
%{_libdir}/libe2p.a
%{_libdir}/libe2p.so
%{_libdir}/libext2fs.a
%{_libdir}/libext2fs.so
%{_libdir}/pkgconfig/e2p.pc
%{_libdir}/pkgconfig/ext2fs.pc
%{_includedir}/e2p
%{_includedir}/ext2fs
%manifest e2fsprogs.manifest

%files -n libcom_err
%defattr(-,root,root)
%{_datadir}/license/libcom_err
%{_root_libdir}/libcom_err.so.*
%manifest e2fsprogs.manifest

%files -n libcom_err-devel
%defattr(-,root,root)
%{_bindir}/compile_et
%{_libdir}/libcom_err.a
%{_libdir}/libcom_err.so
%{_datadir}/et
%{_includedir}/com_err.h
%{_includedir}/et
%{_libdir}/pkgconfig/com_err.pc
%manifest e2fsprogs.manifest

%files -n libss
%defattr(-,root,root)
%{_datadir}/license/libss
%{_root_libdir}/libss.so.*
%manifest e2fsprogs.manifest

%files -n libss-devel
%defattr(-,root,root)
%{_bindir}/mk_cmds
%{_libdir}/libss.a
%{_libdir}/libss.so
%{_datadir}/ss
%{_includedir}/ss
%{_libdir}/pkgconfig/ss.pc
%manifest e2fsprogs.manifest
