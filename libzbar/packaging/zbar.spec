#%{!?python_sitelib: %global python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib()")}
#%{!?python_sitearch: %global python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}

Name:           zbar
Version:        0.10
Release:        21
Summary:        Bar code reader
Group:          Development/Libraries
License:        LGPL-2.1+
Source0:        %{name}-%{version}.tar.bz2
#Patch0:        zbar_update_to_hg.patch
#Patch1:        zbar_use_libv4l.patch
#Patch2:        zbar_choose_supported_format_first.patch
#BuildRoot:     %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

#BuildRequires: autoconf automake libtool python-devel gettext-devel
#BuildRequires: qt4-devel gtk2-devel pygtk2-devel GraphicsMagick-c++-devel
#BuildRequires: libv4l-devel libXv-devel xmlto

BuildRequires:  autoconf
BuildRequires:  automake
BuildRequires:  libtool
BuildRequires:  libXv-devel
BuildRequires:  gettext-devel

%description
A layered barcode scanning and decoding library. Supports EAN, UPC, Code 128,
Code 39 and Interleaved 2 of 5.
Includes applications for decoding captured barcode images and using a video
device (eg, webcam) as a barcode scanner.

%package devel
Group: Development/Libraries
Summary: Bar code library extra development files
Requires: pkgconfig, %{name} = %{version}-%{release}

%description devel
This package contains header files and additional libraries used for
developing applications that read bar codes with this library.

%prep
%setup -q

%build
autoreconf -vfi
%configure --without-imagemagick --without-gtk --without-qt --without-python --without-xshm --without-xv --disable-video

# rpath
sed -i 's|^hardcode_libdir_flag_spec=.*|hardcode_libdir_flag_spec=""|g' libtool
sed -i 's|^runpath_var=LD_RUN_PATH|runpath_var=DIE_RPATH_DIE|g' libtool

make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT INSTALL="install -p"
mkdir -p $RPM_BUILD_ROOT/usr/share/license/%{name}

#Remove .la and .a files
find ${RPM_BUILD_ROOT} -name '*.la' -or -name '*.a' | xargs rm -f

# Remove installed doc
rm -rf $RPM_BUILD_ROOT/usr/share/doc/zbar-0.10/

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%post devel -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%postun devel -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%doc COPYING LICENSE NEWS

#%{_bindir}/zbarimg
#%{_bindir}/zbarcam
%{_libdir}/libzbar.so.*
%{_datadir}/license/%{name}
#%{_mandir}/man1/*


%files devel
%defattr(-,root,root,-)

%{_libdir}/libzbar.so
%{_libdir}/pkgconfig/zbar.pc

%dir %{_includedir}/zbar
%{_includedir}/zbar.h
%{_includedir}/zbar/Exception.h
%{_includedir}/zbar/Symbol.h
%{_includedir}/zbar/Image.h
%{_includedir}/zbar/Scanner.h
%{_includedir}/zbar/Decoder.h
%{_includedir}/zbar/ImageScanner.h
%{_includedir}/zbar/Video.h
%{_includedir}/zbar/Window.h
%{_includedir}/zbar/Processor.h
