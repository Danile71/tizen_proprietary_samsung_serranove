Summary:	LZMA compression utilities
Name:		xz
Version:	5.0.3
Release:	1
License:	LGPL-2.1+ and GPL-2.0+ and GPL-3.0+
Group:		Applications/File
# source created as "make dist" in checked out GIT tree
Source0:	http://tukaani.org/%{name}/xz-%{version}.tar.bz2
URL:		http://tukaani.org/%{name}/
Requires:	%{name}-libs = %{version}-%{release}

%description
XZ Utils are an attempt to make LZMA compression easy to use on free (as in
freedom) operating systems. This is achieved by providing tools and libraries
which are similar to use than the equivalents of the most popular existing
compression algorithms.

LZMA is a general purpose compression algorithm designed by Igor Pavlov as
part of 7-Zip. It provides high compression ratio while keeping the
decompression speed fast.

%package 	libs
Summary:	Libraries for decoding LZMA compression
Group:		System/Libraries
License:	LGPLv2+

%description 	libs
Libraries for decoding files compressed with LZMA or XZ utils.

%package 	devel
Summary:	Devel libraries & headers for liblzma
Group:		Development/Libraries
License:	LGPLv2+
Requires:	%{name}-libs = %{version}-%{release}

%description  devel
Devel libraries and headers for liblzma.

%package 	lzma-compat
Summary:	Older LZMA format compatibility binaries
Group:		System/Libraries
# lz{grep,diff,more} are GPLv2+. Other binaries are LGPLv2+
License:	GPLv2+ and LGPLv2+
Requires:	%{name} = %{version}-%{release}
Obsoletes:	lzma < 5
Provides:	lzma = 5

%description  lzma-compat
The lzma-compat package contains compatibility links for older
commands that deal with the older LZMA format.

%prep
%setup -q

%build
CFLAGS="%{optflags} -D_FILE_OFFSET_BITS=64" \
CXXFLAGS="%{optflags} -D_FILE_OFFSET_BITS=64" \
%configure --disable-static \
    --disable-nls

sed -i 's|^hardcode_libdir_flag_spec=.*|hardcode_libdir_flag_spec=""|g' libtool
sed -i 's|^runpath_var=LD_RUN_PATH|runpath_var=DIE_RPATH_DIE|g' libtool

make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%make_install
rm -rf %{buildroot}/%{_docdir}/%{name}

mkdir -p %{buildroot}/%{_datadir}/license
cp -f COPYING.LGPLv2.1 %{buildroot}/%{_datadir}/license/%{name}-libs

%clean
rm -rf %{buildroot}

%post libs -p /sbin/ldconfig

%postun libs -p /sbin/ldconfig


%docs_package

%files
%defattr(-,root,root,-)
%doc COPYING.*
%{_bindir}/*xz*

%files libs
%defattr(-,root,root,-)
%doc COPYING.*
%{_libdir}/lib*.so.*
%{_datadir}/license/%{name}-libs

%files devel
%defattr(-,root,root,-)
%doc AUTHORS README THANKS COPYING.* ChangeLog
%dir %{_includedir}/lzma
%{_includedir}/lzma/*.h
%{_includedir}/lzma.h
%{_libdir}/*.so
%{_libdir}/pkgconfig/liblzma.pc

%files lzma-compat
%defattr(-,root,root,-)
%{_bindir}/*lz*

