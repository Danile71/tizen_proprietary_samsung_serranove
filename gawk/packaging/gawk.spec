Name:           gawk
Version:        3.1.5
Release:        1
License:        GPL-2.0+
Summary:        The GNU version of the awk text processing utility
Url:            http://www.gnu.org/software/gawk/gawk.html
Group:          Applications/Text
Source0:        ftp://ftp.gnu.org/gnu/gawk/gawk-%{version}.tar.bz2
Source1001:     packaging/gawk.manifest
Patch0:         gawk-3.1.3-getpgrp_void.patch
Patch1:         gawk-3.1.5-free.patch
Patch2:         gawk-3.1.5-fieldwidths.patch
Patch3:         gawk-3.1.5-binmode.patch
Patch4:         gawk-3.1.5-num2str.patch
Patch5:         gawk-3.1.5-wconcat.patch
Patch6:         gawk-3.1.5-internal.patch
Patch7:         gawk-3.1.5-syntaxerror.patch
Patch8:         gawk-3.1.5-numflags.patch
Patch9:         gawk-3.1.5-ipv6.patch
Patch10:        gawk-3.1.5-freewstr.patch
Patch11:        gawk-3.1.5-mbread.patch
BuildRequires:  bison
BuildRequires:  flex
Requires:       /bin/mktemp

%description
The gawk package contains the GNU version of awk, a text processing
utility. Awk interprets a special-purpose programming language to do
quick and easy text pattern matching and reformatting jobs.

Install the gawk package if you need a text processing utility. Gawk is
considered to be a standard Linux tool for processing text.

%prep
%setup -q

# gawk-3.1.3-getpgrp_void.patch
%patch0 -p1
# gawk-3.1.5-free.patch
%patch1 -p1
# gawk-3.1.5-fieldwidths.patch
%patch2 -p1
# gawk-3.1.5-binmode.patch
%patch3 -p1
# gawk-3.1.5-num2str.patch
%patch4 -p1
# gawk-3.1.5-wconcat.patch
%patch5 -p1
# gawk-3.1.5-internal.patch
%patch6 -p1
# gawk-3.1.5-syntaxerror.patch
%patch7 -p1
# gawk-3.1.5-numflags.patch
%patch8 -p1
# gawk-3.1.5-ipv6.patch
%patch9 -p1
# gawk-3.1.5-freewstr.patch
%patch10 -p1
# gawk-3.1.5-mbread.patch
%patch11 -p1

%build
cp %{SOURCE1001} .

%configure \
        --bindir=/bin \
        --disable-man \
        --disable-nls

make %{?_smp_mflags}

%install
%make_install

chmod a-x COPYING
mkdir -p %{buildroot}%{_prefix}/bin
pushd %{buildroot}%{_prefix}/bin
ln -s ../../bin/gawk awk
ln -s ../../bin/gawk gawk
popd

mkdir -p %{buildroot}%{_datadir}/license
cat COPYING >> %{buildroot}%{_datadir}/license/%{name}

%remove_docs

%files
%manifest gawk.manifest
%doc COPYING
%{_datadir}/license/%{name}
/bin/*
%exclude /bin/pgawk*
%{_bindir}/*
%{_libexecdir}/awk
%{_datadir}/awk
