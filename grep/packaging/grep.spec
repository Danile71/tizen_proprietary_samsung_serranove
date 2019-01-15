Summary: The GNU versions of grep pattern matching utilities
Name: grep
Version: 2.5.1a
Release: 62
License: GPL-2.0+
Group: Applications/Text
Source: ftp://ftp.gnu.org/pub/gnu/grep/grep-%{version}.tar.bz2
Source1001: packaging/grep.manifest
Patch0: grep-2.5.1-fgrep.patch
Patch1: grep-2.5.1-bracket.patch
Patch2: grep-2.5-i18n.patch
Patch3: grep-2.5.1-oi.patch
Patch4: grep-2.5.1-manpage.patch
Patch5: grep-2.5.1-color.patch
Patch6: grep-2.5.1-icolor.patch
Patch7: grep-skip.patch
Patch10: grep-2.5.1-egf-speedup.patch
Patch11: grep-2.5.1-dfa-optional.patch
Patch12: grep-2.5.1-tests.patch
Patch13: grep-2.5.1-w.patch
Patch14: grep-P.patch
Patch15: grep-mem-exhausted.patch
Patch16: grep-empty-pattern.patch
Patch17: grep-2.5.1a-pcrewrap.patch
Patch18: grep-2.5.1a-utf8.patch
URL: http://www.gnu.org/software/grep/
BuildRequires: libpcre-devel >= 3.9-10

%description
The GNU versions of commonly used grep utilities.  Grep searches
through textual input for lines which contain a match to a specified
pattern and then prints the matching lines.  GNU's grep utilities
include grep, egrep and fgrep.

You should install grep on your system, because it is a very useful
utility for searching through text.

%prep
%setup -q
%patch0 -p1 -b .fgrep
%patch1 -p1 -b .bracket
%patch2 -p1 -b .i18n
%patch3 -p1 -b .oi
%patch4 -p1 -b .manpage
%patch5 -p1 -b .color
%patch6 -p1 -b .icolor
%patch7 -p1 -b .skip
%patch10 -p1 -b .egf-speedup
%patch11 -p1 -b .dfa-optional
%patch12 -p1 -b .tests
%patch13 -p1 -b .w
%patch14 -p1 -b .P
%patch15 -p1 -b .mem-exhausted
%patch16 -p1 -b .empty-pattern
%patch17 -p1 -b .pcrewrap
%patch18 -p1 -b .utf8
chmod a+x tests/fmbtest.sh
chmod a+x tests/pcrewrap.sh

%build
cp %{SOURCE1001} .
%configure --without-included-regex CPPFLAGS="-I%{_includedir}/pcre" --disable-nls
make %{?_smp_mflags}

%install
rm -rf ${RPM_BUILD_ROOT}
make %{?_smp_mflags} DESTDIR=$RPM_BUILD_ROOT install
mkdir -p $RPM_BUILD_ROOT/bin
mv -f $RPM_BUILD_ROOT%{_bindir}/* $RPM_BUILD_ROOT/bin
rm -rf $RPM_BUILD_ROOT%{_bindir}
rm -rf $RPM_BUILD_ROOT%{_infodir}

# Use symlinks for egrep and fgrep
ln -sf grep $RPM_BUILD_ROOT/bin/egrep
ln -sf grep $RPM_BUILD_ROOT/bin/fgrep

mkdir -p $RPM_BUILD_ROOT%{_datadir}/license
for keyword in LICENSE COPYING COPYRIGHT;
do
	for file in `find %{_builddir} -name $keyword`;
	do
		cat $file >> $RPM_BUILD_ROOT%{_datadir}/license/%{name};
		echo "";
	done;
done

#%check
#make check

%clean
rm -rf ${RPM_BUILD_ROOT}

%docs_package

%files
%manifest grep.manifest
%defattr(-,root,root)
%doc COPYING
%{_datadir}/license/%{name}
/bin/*

