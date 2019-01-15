Name:       libidn
Summary:    Internationalized Domain Name support library
Version:    1.15
Release:    %{?release_prefix:%{release_prefix}.}1.45.%{?dist}%{!?dist:tizen}
VCS:        external/libidn#submit/trunk/20121022.071724-1-g9e165ae3da5f9f3c5e073714ea3856c67f8c8bd0
Group:      System/Libraries
License:    LGPLv2.1+
URL:        http://www.gnu.org/software/libidn/
Source0:    http://ftp.gnu.org/gnu/libidn/%{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig
BuildRequires:  gettext-tools
BuildRequires:  libtool
BuildRequires:  autoconf

%description
GNU Libidn is an implementation of the Stringprep, Punycode and
IDNA specifications defined by the IETF Internationalized Domain
Names (IDN) working group, used for internationalized domain
names.



%package devel
Summary:    Development files for the libidn library
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   glibc-devel

%description devel
This package includes header files and libraries necessary for
developing programs which use the GNU libidn library.



%prep
%setup -q -n %{name}-%{version}


%build

%configure --disable-static \
    --disable-csharp \
    --disable-java \
    --with-pic

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install 

rm -f $RPM_BUILD_ROOT/%_infodir/dir
rm -f %{buildroot}%{_libdir}/libidn.la
rm -f  $RPM_BUILD_ROOT%_infodir/libidn-components.png
rm -f %{buildroot}%{_bindir}/idn
%find_lang libidn

%remove_docs 

mkdir -p %{buildroot}/usr/share/license
cp COPYING.LIB %{buildroot}/usr/share/license/%{name}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files -f libidn.lang
%defattr(-,root,root,-)
%dir %{_datadir}/emacs
%dir %{_datadir}/emacs/site-lisp
%{_datadir}/emacs/site-lisp/idna.el
%{_datadir}/emacs/site-lisp/punycode.el
%{_libdir}/libidn.so.*
/usr/share/license/%{name}

%files devel
%defattr(-,root,root,-)
%{_libdir}/libidn.so
%{_includedir}/*.h
%{_libdir}/pkgconfig/libidn.pc
