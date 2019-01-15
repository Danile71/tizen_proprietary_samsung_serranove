#sbs-git:slp/unmodified/fribidi fribidi 0.19.5 b36fcb1ced2e1294dd147e36f120f5161e53406d

Name:       fribidi
Summary:    Library implementing the Unicode Bidirectional Algorithm
Version: 0.19.6
Release:    5
Group:      System/Libraries
License:    LGPLv2+
URL:        http://fribidi.org
Source0:    http://fribidi.org/download/%{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  automake
BuildRequires:  autoconf
BuildRequires:  libtool
BuildRequires:  pkgconfig


%description
A library to handle bidirectional scripts (for example Hebrew, Arabic),
so that the display is done in the proper way; while the text data itself
is always written in logical order.



%package devel
Summary:    Libraries and include files for FriBidi
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Include files and libraries needed for developing applications which use
FriBidi.



%prep
%setup -q -n %{name}-%{version}


%build

%reconfigure --disable-static
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/usr/share/license
cp %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/usr/share/license/%{name}




%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig





%files
%defattr(-,root,root,-)
%doc README AUTHORS COPYING ChangeLog THANKS NEWS TODO
%{_bindir}/fribidi
%{_libdir}/libfribidi.so.*
%manifest %{name}.manifest
/usr/share/license/%{name}


%files devel
%defattr(-,root,root,-)
%{_includedir}/fribidi
%{_libdir}/libfribidi.so
%{_libdir}/pkgconfig/*.pc
%{_mandir}/man3/%{name}_*.gz

