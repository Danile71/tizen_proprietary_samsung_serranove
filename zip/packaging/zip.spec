Summary: A file compression and packaging utility compatible with PKZIP.
Name: zip
Version: 3.0
Release: 1.0.1
License: distributable
Group: Applications/Archiving
Source:%{name}-%{version}.tar.gz
URL: http://www.info-zip.org/pub/infozip/Zip.html

%description
The zip program is a compression and file packaging utility.  Zip is
analogous to a combination of the UNIX tar and compress commands and
is compatible with PKZIP (a compression and file packaging utility for
MS-DOS systems).

Install the zip package if you need to compress files using the zip
program.

%prep
%setup -q
%build
make -f unix/Makefile prefix=/usr "CFLAGS=$RPM_OPT_FLAGS -I. -DUNIX -D_LARGEFILE64_SOURCE" generic_gcc

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BULD_ROOT%{_mandir}/man1

make -f unix/Makefile prefix=$RPM_BUILD_ROOT/usr \
	MANDIR=$RPM_BUILD_ROOT%{_mandir}/man1 install

pushd $RPM_BUILD_ROOT
for n in zipnote zipsplit zip zipcloak ; do
    chmod 755 ./usr/bin/$n
done
popd

mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc README BUGS CHANGES TODO WHATSNEW WHERE LICENSE
%doc proginfo/algorith.txt
/usr/bin/zipnote
/usr/bin/zipsplit
/usr/bin/zip
/usr/bin/zipcloak
%{_mandir}/man1/zip*1*
/usr/share/license/%{name}
