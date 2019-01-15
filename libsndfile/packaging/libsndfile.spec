#sbs-git:slp/pkgs/l/libsndfile libsndfile 1.0.21 82073f8ea0cf1e3791fea9362087d4f61db9b9c8

Name:       libsndfile
Summary:    Library for reading and writing sound files
Version:    1.0.28
Release:    0
Group:      System/Libraries
License:    LGPLv2+
URL:        http://www.mega-nerd.com/libsndfile/
Source0:    http://www.mega-nerd.com/libsndfile/libsndfile-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
#BuildRequires:  pkgconfig(alsa)
BuildRequires:  pkgconfig(ogg)
BuildRequires:  pkgconfig(vorbis)
#BuildRequires:  pkgconfig(flac)


%description
libsndfile is a C library for reading and writing sound files such as
AIFF, AU, WAV, and others through one standard interface. It can
currently read/write 8, 16, 24 and 32-bit PCM files as well as 32 and
64-bit floating point WAV files and a number of compressed formats. It
compiles and runs on *nix, MacOS, and Win32.



%package devel
Summary:    Development files for libsndfile
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
libsndfile is a C library for reading and writing sound files such as
AIFF, AU, WAV, and others through one standard interface.
This package contains files needed to develop with libsndfile.



%prep
%setup -q -n %{name}-%{version}


%build
%autogen

%configure --disable-static \
    --disable-dependency-tracking \
    --disable-sqlite --disable-alsa

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp COPYING %{buildroot}/usr/share/license/%{name}
%make_install




%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig



%docs_package


%files
%manifest libsndfile.manifest
%defattr(-,root,root,-)
%doc COPYING 
%exclude %{_bindir}/*
%{_libdir}/%{name}.so.*
/usr/share/license/%{name}


%files devel
%defattr(-,root,root,-)
%{_includedir}/sndfile.h
%{_includedir}/sndfile.hh
%{_libdir}/%{name}.so
%{_libdir}/pkgconfig/sndfile.pc
/usr/share/doc/libsndfile1-dev

