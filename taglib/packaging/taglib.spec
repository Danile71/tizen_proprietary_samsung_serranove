Name:      taglib
Summary:   library for reading and editing the meta-data of several popular audio formats
Version:   1.8
Release:   3
Group:     System/Libraries
License:   LGPL-2.1+ or MPL-1.1
Source0:   %{name}-%{version}.tar.gz
BuildRequires:  cmake
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
library for reading and editing the meta-data of several popular audio formats

%package devel
Summary:  A taglib  library (Development)
Group:    Development/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
library for reading and editing the meta-data of several popular audio formats

%prep
%setup -q

%build
MAJORVER=`echo %{version} | awk 'BEGIN {FS="."}{print $1}'`
cmake . -DCMAKE_INSTALL_PREFIX=/usr -DFULLVER=%{version} -DMAJORVER=${MAJORVER}

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/%{_datadir}/license
cp -rf %{_builddir}/%{name}-%{version}/COPYING.LGPL %{buildroot}/%{_datadir}/license/%{name}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%manifest taglib.manifest
%{_libdir}/libtag.so*
%{_libdir}/libtag_c.so*
%{_datadir}/license/%{name}


%files devel
%{_includedir}/taglib/*.h
%{_libdir}/libtag.so*
%{_includedir}/taglib/tlist.tcc
%{_includedir}/taglib/tmap.tcc
%{_libdir}/pkgconfig/taglib.pc
%{_libdir}/pkgconfig/taglib_c.pc
