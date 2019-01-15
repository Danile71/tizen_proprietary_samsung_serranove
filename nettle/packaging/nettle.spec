Name:           nettle
Version:        2.7.1
Release:        0
Summary:        Cryptographic Library
License:        LGPL-2.1+ and GPL-2.0+
Group:          Security/Crypto Libraries
Source0:        ftp://ftp.lysator.liu.se/pub/security/lsh/nettle-%{version}.tar.gz
Source1:        baselibs.conf
Source1001: 	nettle.manifest
BuildRequires:  gmp-devel
BuildRequires:  pkgconfig

%description
Nettle is a cryptographic library that is designed to fit easily in more or
less any context: In crypto toolkits for object-oriented languages (C++,
Python, Pike, ...), in applications like LSH or GNUPG, or even in kernel space.

%package -n libhogweed

Summary:        Cryptographic Library for Public Key Algorithms
License:        LGPL-2.1+
Group:          Security/Crypto Libraries
Provides:		libhogweed.so.2

%description -n libhogweed
Nettle is a cryptographic library that is designed to fit easily in more or
less any context: In crypto toolkits for object-oriented languages (C++,
Python, Pike, ...), in applications like LSH or GNUPG, or even in kernel space.

The libhogweed library contains public key algorithms to use with libnettle.

%package -n libnettle-devel
Summary:        Cryptographic Library
License:        LGPL-2.1+
Group:          Development/Libraries
Requires:       glibc-devel
Requires:       gmp-devel
Requires:       libhogweed
Requires:       libnettle

%description -n libnettle-devel
Nettle is a cryptographic library that is designed to fit easily in more or
less any context: In crypto toolkits for object-oriented languages (C++,
Python, Pike, ...), in applications like LSH or GNUPG, or even in kernel space.

%package -n libnettle

Summary:        Cryptographic Tools
License:        LGPL-2.1+ and GPL-2.0+
Group:          Security/Crypto Libraries
Provides:		libnettle.so.4

%description -n libnettle
Nettle is a cryptographic library that is designed to fit easily in more or
less any context: In crypto toolkits for object-oriented languages (C++,
Python, Pike, ...), in applications like LSH or GNUPG, or even in kernel space.

This package contains a few command-line tools to perform cryptographic
operations using the nettle library.

%prep
%setup -q 
cp %{SOURCE1001} .

%build
#%configure --disable-static \
%configure --enable-shared

make %{?_smp_mflags}

%check
make check

%install
%make_install

%post   -n libnettle -p /sbin/ldconfig

%postun -n libnettle -p /sbin/ldconfig

%post   -n libhogweed -p /sbin/ldconfig

%postun -n libhogweed -p /sbin/ldconfig


%files -n libnettle
%manifest %{name}.manifest
%defattr(-,root,root)
%license COPYING*
%{_libdir}/libnettle.so.*

%files -n libhogweed
%manifest %{name}.manifest
%defattr(-,root,root)
%{_libdir}/libhogweed.so.*

%files -n libnettle-devel
%manifest %{name}.manifest
%defattr(-,root,root)
%{_includedir}/nettle
%{_libdir}/libnettle.so
%{_libdir}/libhogweed.so
%{_libdir}/pkgconfig/hogweed.pc
%{_libdir}/pkgconfig/nettle.pc

%files -n nettle
%manifest %{name}.manifest
%defattr(-,root,root)
%{_bindir}/nettle-lfib-stream
%{_bindir}/pkcs1-conv
%{_bindir}/sexp-conv
%{_bindir}/nettle-hash

%changelog
