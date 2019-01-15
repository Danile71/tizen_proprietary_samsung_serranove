Name:           dosfstools
Provides:       mkdosfs dosfsck
License:        GPL v2 or later
Group:          System/Filesystems
AutoReqProv:    on
Summary:        Utilities for Making and Checking MS-DOS FAT File Systems on Linux
Version:        2.11
Release:        1
Url:            ftp://ftp.uni-erlangen.de/pub/Linux/LOCAL/dosfstools
Source:         %{name}-%{version}.tar.bz2
Source1001:     %{name}.manifest
Patch0:         %{name}-%{version}-linuxfs.patch
Patch1:         %{name}-%{version}-unaligned.patch
Patch2:         %{name}-%{version}-buffer.patch
Patch3:         %{name}-%{version}-o_excl.patch
Patch4:         %{name}-%{version}-mkdosfs-geo0.diff
Patch5:         %{name}-%{version}_determine-sector-size.patch
Patch6:         %{name}-%{version}-unsupported-sector-size.patch
Obsoletes:      mkdosfs dosfsck dosfstls
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
#Supplements:    filesystem(vfat)

%description
The dosfstools package includes the mkdosfs and dosfsck utilities,
which respectively make and check MS-DOS FAT file systems on hard
drives or on floppies.



Authors:
--------
    Dave Hudson <dave@humbug.demon.co.uk>
    Werner Almesberger <werner.almesberger@lrc.di.epfl.ch>
    Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>

%prep
%setup
%patch0
%patch1 -p1
%patch2
%patch3
%patch4 -p1
%patch5
%patch6

%build
cp %{SOURCE1001} .
make OPTFLAGS="-D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE $RPM_OPT_FLAGS"

%install
# directories
install -d $RPM_BUILD_ROOT{/sbin,%{_mandir}/man8}
# binaries
install -m755 mkdosfs/mkdosfs $RPM_BUILD_ROOT/sbin/
install -m755 dosfsck/dosfsck $RPM_BUILD_ROOT/sbin/
# alternative names
ln -sf mkdosfs $RPM_BUILD_ROOT/sbin/mkfs.msdos
ln -sf dosfsck $RPM_BUILD_ROOT/sbin/fsck.msdos
ln -sf mkdosfs $RPM_BUILD_ROOT/sbin/mkfs.vfat
ln -sf dosfsck $RPM_BUILD_ROOT/sbin/fsck.vfat
# man pages
install -m 644 mkdosfs/mkdosfs.8 $RPM_BUILD_ROOT%{_mandir}/man8/
install -m 644 dosfsck/dosfsck.8 $RPM_BUILD_ROOT%{_mandir}/man8/
# man pages for alternative names
ln -sf mkdosfs.8.gz $RPM_BUILD_ROOT%{_mandir}/man8/mkfs.msdos.8.gz
ln -sf dosfsck.8.gz $RPM_BUILD_ROOT%{_mandir}/man8/fsck.msdos.8.gz
ln -sf mkdosfs.8.gz $RPM_BUILD_ROOT%{_mandir}/man8/mkfs.vfat.8.gz
ln -sf dosfsck.8.gz $RPM_BUILD_ROOT%{_mandir}/man8/fsck.vfat.8.gz
# documentation
install -m755 -d $RPM_BUILD_ROOT/%{_docdir}/%{name}/dosfsck
install -m755 -d $RPM_BUILD_ROOT/%{_docdir}/%{name}/mkdosfs
install -m644 CHANGES TODO README.Atari $RPM_BUILD_ROOT/%{_docdir}/%{name}/
install -m644 dosfsck/{COPYING,README} $RPM_BUILD_ROOT/%{_docdir}/%{name}/dosfsck
install -m644 mkdosfs/{COPYING,README} $RPM_BUILD_ROOT/%{_docdir}/%{name}/mkdosfs

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%manifest %{name}.manifest
%doc %{_docdir}/%{name}
/sbin/*
%{_mandir}/man8/*.gz
