Name:           libxslt
Version:        1.1.28
Release:        1
Summary:        XSL Transformation Library
License:        MIT
Group:          System/Libraries
Url:            http://xmlsoft.org/XSLT/
Source:         %{name}-%{version}.tar.bz2
#X-Vcs-Url:     git://git.gnome.org/libxslt
#Source2:        baselibs.conf
Source3:        xslt-config.1.gz
Source1001:     libxslt.manifest
#BuildRequires:  libgcrypt-devel
#BuildRequires:  libgpg-error-devel
BuildRequires:  libtool
BuildRequires:  libxml2-devel
BuildRequires:  pkg-config
#Patch1:         libxslt_aslr_20150408.patch

%description
This C library allows you to transform XML files into other XML files
(or HTML, text, and more) using the standard XSLT stylesheet
transformation mechanism.

It is based on libxml (version 2) for XML parsing, tree manipulation,
and XPath support. It is written in plain C, making as few assumptions
as possible and sticks closely to ANSI C/POSIX for easy embedding.
Although not primarily designed with performance in mind, libxslt seems
to be a relatively fast processor. It also includes full support for
the EXSLT set of extension functions as well as some common extensions
present in other XSLT engines.


%package devel
Summary:        Include Files and Libraries mandatory for Development
Group:          System/Libraries
Requires:       %{name}-tools = %version
Requires:       libxslt = %{version}
Requires:       glibc-devel
#Requires:       libgcrypt-devel
#Requires:       libgpg-error-devel
#libxml is automatically required with pkgconfig

%description devel
This package contains all necessary include files and libraries needed
to develop applications that require these.

%package tools
Summary:        Extended Stylesheet Language (XSL) Transformation utilities
Group:          Development/Tools
Provides:       xsltproc = %version-%release

%description tools
This package contains xsltproc, a command line interface to the XSLT engine.

%prep
%setup -q
cp %{SOURCE1001} .
#%patch1 -p1 -b .aslr

%build
#export CFLAGS="%optflags -fPIC"
#export CXXFLAGS="${CXXFLAGS:-%optflags} -fPIC"
%autogen --disable-static --with-pic --without-python
%__make %{?_smp_mflags}

%check
%if ! 0%{?qemu_user_space_build}
%__make check
%endif

%install
%make_install

# Unwanted doc stuff
rm -fr %{buildroot}%{_datadir}/doc

# the manual page is required
install -ma=r '-t%{buildroot}%{_mandir}/man1' '%{SOURCE3}'

mkdir -p %{buildroot}/usr/share/license
cp -af Copyright %{buildroot}/usr/share/license/%{name}

%post -n libxslt -p /sbin/ldconfig

%postun -n libxslt -p /sbin/ldconfig

%files -n libxslt
%manifest %{name}.manifest
%defattr(-, root, root)
%{_libdir}/lib*.so.*
%{_datadir}/license/%{name}

%files devel
%manifest %{name}.manifest
%defattr(-, root, root)
%{_libdir}/lib*.so
%{_libdir}/*.sh
%{_libdir}/pkgconfig/*.pc
%{_includedir}/*
%{_datadir}/aclocal/*
%{_bindir}/xslt-config
%doc %{_mandir}/man1/xslt-config.*
%doc %{_mandir}/man3/*

%files tools
%manifest %{name}.manifest
%defattr(-,root,root)
%{_bindir}/xsltproc
%doc %{_mandir}/man1/xsltproc.*
