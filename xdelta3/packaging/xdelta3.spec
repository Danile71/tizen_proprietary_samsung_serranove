Summary: 	xdelta3
Name: 		xdelta3
Version: 		3.0.9
Release: 		1
License: 		GPL v2.0
Group: 		System/Libraries
Source:		%{name}-%{version}.tar.gz
SOURCE1:	xdelta3.manifest
URL:			http://xdelta.org/

%description
Xdelta3 is a set of tools designed to compute changes between
binary files.  These changes (delta files) are similar to the output of the
"diff" program, in that they may be used to store and transmit only the
changes between files.  The "delta files" that Xdelta3 manages are
stored in RFC3284 (VCDIFF) format.

%package devel
Summary:	Development files for xdelta3
Group:		Development/Libraries
Requires:		%{name} = %{version}-%{release}

%description devel
This package contains the header files, libraries and documentation needed to
develop applications that use xdelta3.

%prep
%setup -q

%build
cp %{SOURCE1} .
libtoolize -c -f
aclocal
autoheader
autoconf
automake -a -c
./configure --prefix="/usr"
make %{?jobs:-j%jobs} xdelta3

%install
rm -rf %{buildroot}
%make_install

install -D -m 0644 COPYING %{buildroot}/usr/share/license/xdelta3

%clean
rm -rf $RPM_BUILD_ROOT

%files
%manifest xdelta3.manifest
%defattr(-, root, root)
%doc README COPYING
%doc %{_mandir}/man1/xdelta3.1.gz
%doc /usr/share/license/xdelta3
%{_bindir}/%{name}
