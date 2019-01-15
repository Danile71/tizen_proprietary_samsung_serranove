Summary: Utilities for managing filesystem extended attributes
Name: attr
Version: 2.4.44
Release: %{?release_prefix:%{release_prefix}.}7.11.%{?dist}%{!?dist:tizen}
VCS:     framework/base/attr#Z910F_PROTEX_0625-2-gf85ff3a6295796e796f7a57e540601bd2ca3bd86
Source: http://download.savannah.gnu.org/releases/attr/attr-%{version}.src.tar.gz
Source1001: attr.manifest
Patch0: 01-578386-Makefile.patch
Patch1: 02-fix_memory_leak_in_attr_copy_action.patch
Patch2: 03-pull_in_string.h.patch
Patch3: 05-eliminate_a_memory_leak.patch
Patch4: 06-eliminate_a_double_free.patch
Patch5: 07-fix_thinko_in_restore.patch
Patch6: attr-2.4.44-build.patch
Patch7: 08-getfattr_encode_NULs_properly_with_--encoding=text.patch

License: GPLv2+
URL: http://savannah.nongnu.org/projects/attr/
Group: System/Base
BuildRequires: autoconf
BuildRequires: libtool >= 1.5
BuildRequires: gettext-tools

%description
This package contains the libattr.so dynamic library which contains
the extended attribute system calls and library functions.

%package -n libattr
Summary: Utilities for managing filesystem extended attributes
Group: System/Libraries
License: LGPLv2+

%description -n libattr
Utilities for managing filesystem extended attributes

%package -n libattr-devel
Summary: Extended attribute static libraries and headers
Group: Development/Libraries
License: LGPLv2+
Requires: libattr = %{version}-%{release}

%description -n libattr-devel
This package contains the libraries and header files needed to
develop programs which make use of extended attributes.
For Linux programs, the documented system call API is the
recommended interface, but an SGI IRIX compatibility interface
is also provided.

Currently only ext2, ext3 and XFS support extended attributes.
The SGI IRIX compatibility API built above the Linux system calls is
used by programs such as xfsdump(8), xfsrestore(8) and xfs_fsr(8).

You should install libattr-devel if you want to develop programs
which make use of extended attributes.  If you install libattr-devel,
you'll also want to install attr.

%prep
%setup -q 
%patch0 -p1
%patch1 -p1
%patch2 -p1
%patch3 -p1
%patch4 -p1
%patch5 -p1
%patch6 -p1
%patch7 -p1

%build
cp %{SOURCE1001} .
#export DIST_ROOT=%{buildroot}
export INSTALL_USER="root"
export INSTALL_GROUP="root"
%configure \
 	--prefix=/ \
 	--exec-prefix=/ \
 	--sbindir=/bin \
 	--bindir=/usr/bin \
 	--libdir=/%{_lib} \
 	--libexecdir=/%{_libdir}
make configure
make default %{?_smp_mflags}
rm -f po/attr.pot
make -C po attr.pot

%install
rm -rf $RPM_BUILD_ROOT

DIST_ROOT=%{buildroot}; export DIST_ROOT; make -C . install
DIST_ROOT=%{buildroot}; export DIST_ROOT; make -C . install-dev
DIST_ROOT=%{buildroot}; export DIST_ROOT; make -C . install-lib

chmod +x %{buildroot}/%{_lib}/libattr.so.*
rm -f %{buildroot}/%{_lib}/libattr.{la,a}
rm -f %{buildroot}%{_libdir}/libattr.{la,a}

mkdir -p %{buildroot}/%{_datadir}/license
cp -f COPYING.GPLv2 %{buildroot}/%{_datadir}/license/%{name}
cp -f COPYING.LGPLv2.1 %{buildroot}/%{_datadir}/license/lib%{name}

%remove_docs

%post -n libattr -p /sbin/ldconfig

%postun -n libattr -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%manifest attr.manifest
%{_bindir}/*
%{_prefix}/share/locale/*/LC_MESSAGES/attr.mo
%{_datadir}/license/%{name}

%files -n libattr
%defattr(-,root,root,-)
/%{_lib}/libattr.so.*
%{_datadir}/license/lib%{name}

%files -n libattr-devel
%defattr(-,root,root,-)
/%{_lib}/libattr.so
%{_libdir}/libattr.so
%{_includedir}/attr/*.h
