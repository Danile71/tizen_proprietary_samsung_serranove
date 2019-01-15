Name:       gzip
Summary:    The GNU data compression program
Version:    1.3.12
Release:    1
Group:      Applications/File
License:    GPL-2.0+
URL:        http://www.gzip.org/
Source0:    ftp://alpha.gnu.org/gnu/gzip/gzip-%{version}.tar.gz
Source1001: packaging/gzip.manifest
Requires:   /bin/mktemp


%description
The gzip package contains the popular GNU gzip data compression
program. Gzipped files have a .gz extension.

Gzip should be installed on your system, because it is a
very commonly used data compression program.



%package -n zless
Summary:    file perusal filter for crt viewing of compressed text
License:    GPL-2.0+
Group:      Applications/File
Requires:   %{name} = %{version}-%{release}
Requires:   /usr/bin/less

%description -n zless
Zless  is  a filter which allows examination of compressed or plain text files one
screenful at a time on a soft-copy terminal.  It is the equivalent of setting the
environment variable LESSOPEN to '|gzip -cdfq -- %s', and the environment variable
LESSMETACHARS to and then running less.  However,  enough people seem to think that
having the command zless available is important to be worth providing it.



%prep
%setup -q -n %{name}-%{version}

%build
cp %{SOURCE1001} .
export CPPFLAGS="-DHAVE_LSTAT"

%configure \
	--disable-static \
	--bindir=/bin

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}%{_bindir}
ln -sf ../../bin/gzip %{buildroot}%{_bindir}/gzip
ln -sf ../../bin/gunzip %{buildroot}%{_bindir}/gunzip

for i in  zcmp zegrep zforce zless znew gzexe zdiff zfgrep zgrep zmore ; do
mv %{buildroot}/bin/$i %{buildroot}%{_bindir}/$i
done

# we don't ship it, so let's remove it from ${RPM_BUILD_ROOT}
rm -f %{buildroot}%{_infodir}/dir
# uncompress is a part of ncompress package
rm -f %{buildroot}/bin/uncompress

mkdir -p $RPM_BUILD_ROOT%{_datadir}/license
cat COPYING >> $RPM_BUILD_ROOT%{_datadir}/license/gzip
cat COPYING >> $RPM_BUILD_ROOT%{_datadir}/license/zless

%docs_package

%files
%defattr(-,root,root,-)
%{_datadir}/license/gzip
/bin/*
%{_bindir}/gzip
%{_bindir}/gunzip
%{_bindir}/zcmp
%{_bindir}/zegrep
%{_bindir}/zforce
%{_bindir}/znew
%{_bindir}/gzexe
%{_bindir}/zdiff
%{_bindir}/zfgrep
%{_bindir}/zgrep
%{_bindir}/zmore
%manifest gzip.manifest


%files -n zless
%defattr(-,root,root,-)
%{_datadir}/license/zless
%{_bindir}/zless
%manifest gzip.manifest

