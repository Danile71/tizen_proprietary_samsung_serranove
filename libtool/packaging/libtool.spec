#
# Please submit bugfixes or comments via http://bugs.meego.com/
#

Name:           libtool
Version:        2.2.6b
Release:        %{?release_prefix:%{release_prefix}.}1.11.%{?dist}%{!?dist:tizen}
VCS:            external/libtool#submit/trunk/20121019.111612-5-g3f27a11eef34b1c0d32621c6b45bce898919b792
License:        GPLv2+ and LGPLv2+ and GFDL
Summary:        The GNU Portable Library Tool
Url:            http://www.gnu.org/software/libtool/
Group:          Development/Tools
Source:         http://ftp.gnu.org/gnu/libtool/libtool-%{version}.tar.gz
Patch0:         no-host-name.patch
Patch1:		fix-AC_LANG_PROGRAM.patch
Patch2:		as-needed.patch

BuildRequires:  autoconf >= 2.59
BuildRequires:  automake >= 1.9.2

Requires:       autoconf >= 2.58
Requires:       automake >= 1.4
Requires:       sed

%description
GNU Libtool is a set of shell scripts which automatically configure UNIX and
UNIX-like systems to generically build shared libraries. Libtool provides a
consistent, portable interface which simplifies the process of using shared
libraries.

If you are developing programs which will use shared libraries, but do not use
the rest of the GNU Autotools (such as GNU Autoconf and GNU Automake), you
should install the libtool package.

The libtool package also includes all files needed to integrate the GNU
Portable Library Tool (libtool) and the GNU Libtool Dynamic Module Loader
(ltdl) into a package built using the GNU Autotools (including GNU Autoconf
and GNU Automake).

%package ltdl
License:        LGPLv2+
Summary:        Runtime libraries for GNU Libtool Dynamic Module Loader
Group:          System/Libraries
Provides:       %{name}-libs = %{version}
Requires(post):  /sbin/ldconfig
Requires(postun):  /sbin/ldconfig

%description ltdl
The libtool-ltdl package contains the GNU Libtool Dynamic Module Loader, a
library that provides a consistent, portable interface which simplifies the
process of using dynamic modules.

These runtime libraries are needed by programs that link directly to the
system-installed ltdl libraries; they are not needed by software built using
the rest of the GNU Autotools (including GNU Autoconf and GNU Automake).

%package ltdl-devel
License:        LGPLv2+
Summary:        Tools needed for development using the GNU Libtool Dynamic Module Loader
Group:          Development/Libraries
Requires:       %{name}-ltdl = %{version}

%description ltdl-devel
Static libraries and header files for development with ltdl.

%prep
%setup -n libtool-%{version} -q
%patch0 -p1
%patch1 -p1
%patch2 -p1

%build

./bootstrap

export CC=gcc
export CXX=g++
export F77=gfortran
export CFLAGS="%{optflags} -fPIC"
# don't conflict with libtool-1.5, use own directory:
sed -e 's/pkgdatadir="\\${datadir}\/\$PACKAGE"/pkgdatadir="\\${datadir}\/\${PACKAGE}"/' configure > configure.tmp; mv -f configure.tmp configure; chmod a+x configure
./configure --prefix=%{_prefix} --exec-prefix=%{_prefix} --bindir=%{_bindir} --sbindir=%{_sbindir} --sysconfdir=%{_sysconfdir} --datadir=%{_datadir} --includedir=%{_includedir} --libdir=%{_libdir} --libexecdir=%{_libexecdir} --localstatedir=%{_localstatedir} --mandir=%{_mandir} --infodir=%{_infodir}
# build not smp safe:
make 


%install
%make_install
rm -rf %{buildroot}%{_infodir}

mkdir -p %{buildroot}/usr/share/license
cp COPYING %{buildroot}/usr/share/license/%{name}
cp COPYING %{buildroot}/usr/share/license/%{name}-ltdl

%check
#make check VERBOSE=yes > make_check.log 2>&1 || (cat make_check.log && false)


%clean
rm -rf %{buildroot}

%post ltdl -p /sbin/ldconfig

%postun ltdl -p /sbin/ldconfig

%files
%defattr(-,root,root)
%doc AUTHORS COPYING  THANKS 
%{_bindir}/libtool
%{_bindir}/libtoolize
%{_datadir}/aclocal/*.m4
%exclude %{_datadir}/libtool/libltdl
%{_datadir}/libtool
/usr/share/license/%{name}

%files ltdl
%defattr(-,root,root)
%doc libltdl/COPYING.LIB libltdl/README
%{_libdir}/libltdl.so.*
/usr/share/license/%{name}-ltdl

%files ltdl-devel
%defattr(-,root,root)
%{_datadir}/libtool/libltdl
%{_libdir}/libltdl.so
%{_includedir}/ltdl.h
%{_includedir}/libltdl

