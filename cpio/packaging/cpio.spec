Name:           cpio
Version:        2.8
Release:        3
License:        GPL-3.0+
Summary:        A GNU archiving program
Url:            http://www.gnu.org/software/cpio/
Group:          Applications/Archiving
Source0:        ftp://ftp.gnu.org/gnu/cpio/cpio-%{version}.tar.gz
Source1:        cpio.1
Source1001:     cpio.manifest
BuildRequires:  autoconf

%description
GNU cpio copies files into or out of a cpio or tar archive.  Archives
are files which contain a collection of other files plus information
about them, such as their file name, owner, timestamps, and access
permissions.  The archive can be another file on the disk, a magnetic
tape, or a pipe.  GNU cpio supports the following archive formats:  binary,
old ASCII, new ASCII, crc, HPUX binary, HPUX old ASCII, old tar and POSIX.1
tar.  By default, cpio creates binary format archives, so that they are
compatible with older cpio programs.  When it is extracting files from
archives, cpio automatically recognizes which kind of archive it is reading
and can read archives created on machines with a different byte-order.

Install cpio if you need a program to manage file archives.

%prep
%setup -q



%build
cp %{SOURCE1001} .
export ac_cv_prog_cc_c99=no
%configure --disable-nls

make %{?_smp_mflags}

%install
%make_install
mkdir -p %{buildroot}%{_mandir}/man1
cp -a %{SOURCE1} %{buildroot}%{_mandir}/man1


mkdir -p %{buildroot}/bin
ln -sf ../usr/bin/cpio %{buildroot}/bin/
rm -rf %{buildroot}%{_libexecdir}/rmt

mkdir -p %{buildroot}%{_datadir}/license
cat COPYING >> %{buildroot}%{_datadir}/license/%{name}

%docs_package

%files
%manifest cpio.manifest
%doc COPYING
%{_datadir}/license/%{name}
%{_bindir}/*
/bin/cpio


