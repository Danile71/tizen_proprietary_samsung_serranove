Name:         gmp
Summary:      GNU Arbitrary Precision Arithmetic Library
URL:          https://gmplib.org/
Vendor:       Free Software Foundation
Group:        Algorithm
License:      GPLv2 and LGPLv2.1
Version:      4.2.1
Release:      1

#   list of sources
Source0:      ftp://ftp.gnu.org/gnu/gmp/%{name}-%{version}.tar.gz
Patch0:       gmp.patch
Patch1:       gmp-h-cpp.patch

%description
GNU MP is a library for arbitrary precision arithmetic, operating
on signed integers, rational numbers, and floating point numbers.
It has a rich set of functions, and the functions have a regular
interface. GNU MP is designed to be as fast as possible, both for
small operands and for huge operands. The speed is achieved by using
fullwords as the basic arithmetic type, by using fast algorithms, by
carefully optimized assembly code for the most common inner loops
for a lot of CPUs, and by a general emphasis on speed (instead of
simplicity or elegance).

%package devel
Summary: Development tools for the GNU MP arbitrary precision library
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
The libraries, header files and documentation for using the GNU MP 
arbitrary precision library in applications.

If you want to develop applications which will use the GNU MP library,
you'll need to install the gmp-devel package.  You'll also need to
install the gmp package.

%package static
Summary: Development tools for the GNU MP arbitrary precision library
Group: Development/Libraries
Requires: %{name}-devel = %{version}-%{release}

%description static
The static libraries for using the GNU MP arbitrary precision library 
in applications.

%prep
%setup -q
%patch0 -p0
%patch1 -p1
rm -rf mpn/sparc*

%build
export LDFLAGS+="-Wl,-z,noexecstack"
./configure \
    --prefix=%{_prefix} \
    --includedir=%{_includedir} \
    --libdir=%{_libdir} \
    --enable-cxx \
    --enable-mpbsd

make %{?jobs:-j%jobs}

%install
rm -rf $RPM_BUILD_ROOT
make install AM_MAKEFLAGS="DESTDIR=$RPM_BUILD_ROOT"
find $RPM_BUILD_ROOT -name "*.la" -exec rm -f {} \;
rm -rf $RPM_BUILD_ROOT/%{_prefix}/info/

%remove_docs

mkdir -p %{buildroot}/usr/share/license
cp -f COPYING %{buildroot}/usr/share/license/%{name}
cp -f COPYING.LIB %{buildroot}/usr/share/license/%{name}-lib

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_libdir}/libgmp.so.*
%{_libdir}/libgmpxx.so.*
%{_libdir}/libmp.so.*
/usr/share/license/%{name}*

%files devel
%defattr(-,root,root,-)
%{_includedir}/*.h
%{_libdir}/libgmp.so
%{_libdir}/libgmpxx.so
%{_libdir}/libmp.so

%files static
%defattr(-,root,root,-)
%{_libdir}/libgmp.a
%{_libdir}/libgmpxx.a
%{_libdir}/libmp.a
