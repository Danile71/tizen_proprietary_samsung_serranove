Name:       libexif
Summary:    Library for extracting extra information from image files
Version:    0.6.22
Release:    16
VCS:        framework/multimedia/libexif#libexif_0.6.21-5-2-g80e7b14e48db02bcff587ce87a1e3d7520d5ce29
Group:      System/Libraries
License:    LGPL-2.1+
#URL:        http://libexif.sourceforge.net/
# somewhat heavily modified to add Samsung's own tags for some reason...
Source0:    %{name}-%{version}-samsung.tar.gz

Requires(post):  /sbin/ldconfig
Requires(postun):  /sbin/ldconfig

BuildRoot:  %{_tmppath}/%{name}-%{version}-build

%description
Most digital cameras produce EXIF files, which are JPEG files with
extra tags that contain information about the image. The EXIF library
allows you to parse an EXIF file and read the data from those tags.


%package devel
Summary:    Files needed for libexif application development
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
The libexif-devel package contains the libraries and header files
for writing programs that use libexif.

%prep
%setup -q -n %{name}-%{version}-samsung

%build
sh ./autogen.sh

export CFLAGS+=" -DGST_EXT_EXIF_MODIFICATION"

%configure --disable-static \
    --disable-docs \
    --disable-nls

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp COPYING %{buildroot}/usr/share/license/%{name}
%make_install
#%find_lang libexif-12

rm -rf $RPM_BUILD_ROOT%{_datadir}/doc/libexif

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
#%files -f libexif-12.lang
%manifest libexif.manifest
%{_libdir}/*.so*
%{_datadir}/license/%{name}

%files devel
%{_includedir}/libexif
%{_libdir}/*.so
%{_libdir}/pkgconfig/libexif.pc
