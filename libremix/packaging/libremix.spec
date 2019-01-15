%define prefix  /usr

Name:       libremix
Summary:    An audio sequencing and mixing library.
Version:    0.2.4+build14
Release:    1
Group:      Libraries/Sound
License:    LGPLv2.1
URL:        http://www.metadecks.org/software/remix/
Source0:    %{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires: libsndfile-devel

%description
Remix is a library for rendering audio data.

%package devel
Summary: Libraries, includes, etc to develop remix applications
Group: Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Libraries, include files, etc you can use to develop remix applications.

%prep
%setup -q

%build
./autogen.sh
./configure --prefix=%{prefix}
make %{?jobs:-j%jobs}

%install
if [ -d %{buildroot} ]; then rm -rf %{buildroot}; fi
make install DESTDIR=%{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/usr/share/license/%{name}

%clean
if [ -d %{buildroot} ]; then rm -rf %{buildroot}; fi

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%doc AUTHORS COPYING ChangeLog INSTALL NEWS README TODO doc
%{_libdir}/libremix.so.*
%{_libdir}/libctxdata.so*
%manifest %{name}.manifest
/usr/share/license/%{name}

%files devel
%defattr(-,root,root,-)
%{_libdir}/libremix.a
%{_libdir}/libremix.la
%{_libdir}/libremix.so
%{_libdir}/libctxdata.a
%{_libdir}/libctxdata.la
%{_libdir}/libctxdata.so*
%{_libdir}/pkgconfig/*.pc
%{_libdir}/remix/libremix_ladspa*
%{_libdir}/remix/libremix_noise*
%{_includedir}/ctxdata.h
%{_includedir}/remix/*.h
