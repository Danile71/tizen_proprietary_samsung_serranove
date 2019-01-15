#sbs-git:slp/pkgs/n/ncurses ncurses 5.7+20100313-2slp2+s1 b27eeeb3693466f3c0f5fb528155754f3858d14f
Name:       ncurses
Summary:    Ncurses support utilities
Version:    5.7
Release:    4
Group:      System/Base
License:    GPL-2.0+
URL:        http://invisible-island.net/ncurses/ncurses.html
Source0:    http://ftp.gnu.org/pub/gnu/ncurses/ncurses-%{version}.tar.gz
Source101:  ncurses-rpmlintrc
Patch0:     01-use-d-reentrant.patch
Patch1:     02-debian-backspace.patch
Patch2:     03-linux-use-fsuid.patch
Patch3:     05-emdebian-wchar.patch
Patch4:     06-kfreebsd.patch
Patch5:     08-pkg-config-libdir.patch
BuildRequires:  pkgconfig

%description
The curses library routines are a terminal-independent method of
updating character screens with reasonable optimization.  The ncurses
(new curses) library is a freely distributable replacement for the
discontinued 4.4 BSD classic curses library.

This package contains support utilities, including a terminfo compiler
tic, a decompiler infocmp, clear, tput, tset, and a termcap conversion
tool captoinfo.



%package -n ncurses-libs
Summary:    Ncurses libraries
Group:      System/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   ncurses-base = %{version}-%{release}
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description -n ncurses-libs
The curses library routines are a terminal-independent method of
updating character screens with reasonable optimization.  The ncurses
(new curses) library is a freely distributable replacement for the
discontinued 4.4 BSD classic curses library.

This package contains the ncurses libraries.


%package term
Summary:    Terminal descriptions
Group:      System/Base
Requires:   %{name} = %{version}-%{release}
Requires:   ncurses-base = %{version}-%{release}

%description term
This package contains additional terminal descriptions not found in
the ncurses-base package.


%package base
Summary:    Descriptions of common terminals
Group:      System/Base
Requires:   %{name} = %{version}-%{release}
Conflicts:   ncurses < 5.6-13

%description base
This package contains descriptions of common terminals. Other terminal
descriptions are included in the ncurses-term package.


%package -n ncurses-devel
Summary:    Development files for the ncurses library
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   ncurses-libs = %{version}-%{release}

%description -n ncurses-devel
The header files and libraries for developing applications that use
the ncurses terminal handling library.

Install the ncurses-devel package if you want to develop applications
which will use ncurses.



%prep
%setup -q -n %{name}-%{version}

%patch0 -p1
%patch1 -p1
%patch2 -p1
%patch3 -p1
%patch4 -p1
%patch5 -p1

%build

%configure 


%define rootdatadir /lib
export PKG_CONFIG_LIBDIR=%{_libdir}/pkgconfig

%configure --prefix=/usr \
                --with-shared \
                --mandir=/usr/share/man \
                --without-profile --without-debug \
                --disable-rpath --enable-echo \
                --enable-const \
                --without-ada \
                --enable-symlinks \
                --enable-pc-files \
                --disable-lp64 \
                --with-chtype='long' \
                --with-mmask-t='long' \
                --disable-termcap \
                --with-default-terminfo-dir=/usr/share/terminfo \
                --with-terminfo-dirs="/etc/terminfo:/lib/terminfo:/usr/share/terminfo" \
                --with-ticlib

make %{?_smp_mflags} libs
make %{?_smp_mflags} -C progs

%install
rm -rf %{buildroot}


make DESTDIR=$RPM_BUILD_ROOT install.{libs,progs,data}

chmod 755 ${RPM_BUILD_ROOT}%{_libdir}/lib*.so.*.*

# move lib{ncurses{,w},tinfo}.so.* to /lib*
#mkdir $RPM_BUILD_ROOT/%{_lib}
#mv $RPM_BUILD_ROOT%{_libdir}/lib{ncurses{,w},tinfo}.so.* $RPM_BUILD_ROOT/%{_lib}
#for l in $RPM_BUILD_ROOT%{_libdir}/lib{ncurses{,w},tinfo}.so; do
#ln -sf $(echo %{_libdir} | \
#sed 's,\(^/\|\)[^/][^/]*,..,g')/%{_lib}/$(readlink $l) $l
#done

mkdir -p $RPM_BUILD_ROOT{%{rootdatadir},%{_sysconfdir}}/terminfo

# move few basic terminfo entries to /lib
baseterms=
for termname in \
ansi dumb linux vt100 vt100-nav vt102 vt220 vt52
do
for t in $(find $RPM_BUILD_ROOT%{_datadir}/terminfo \
-samefile $RPM_BUILD_ROOT%{_datadir}/terminfo/${termname::1}/$termname)
do
baseterms="$baseterms $(basename $t)"
done
done
for termname in $baseterms; do
termpath=terminfo/${termname::1}/$termname
mkdir $RPM_BUILD_ROOT%{rootdatadir}/terminfo/${termname::1} &> /dev/null || :
mv $RPM_BUILD_ROOT%{_datadir}/$termpath $RPM_BUILD_ROOT%{rootdatadir}/$termpath
ln -s $(dirname %{_datadir}/$termpath | \
sed 's,\(^/\|\)[^/][^/]*,..,g')%{rootdatadir}/$termpath \
$RPM_BUILD_ROOT%{_datadir}/$termpath
done

# prepare -base and -term file lists
for termname in \
Eterm\* aterm cons25 cygwin eterm\* gnome gnome-256color hurd jfbterm \
konsole konsole-256color mach\* mlterm mrxvt nsterm putty\* pcansi \
rxvt rxvt-\* screen screen-\* screen.\* sun teraterm teraterm2.3 \
wsvt25\* xfce xterm xterm-\*
do
for i in $RPM_BUILD_ROOT%{_datadir}/terminfo/?/$termname; do
for t in $(find $RPM_BUILD_ROOT%{_datadir}/terminfo -samefile $i); do
baseterms="$baseterms $(basename $t)"
done
done
done 2> /dev/null
for t in $baseterms; do
echo "%dir %{_datadir}/terminfo/${t::1}"
echo %{_datadir}/terminfo/${t::1}/$t
done 2> /dev/null | sort -u > terms.base
find $RPM_BUILD_ROOT%{_datadir}/terminfo \! -type d | \
sed "s|^$RPM_BUILD_ROOT||" | while read t
do
echo "%dir $(dirname $t)"
echo $t
done 2> /dev/null | sort -u | comm -2 -3 - terms.base > terms.term

# can't replace directory with symlink (rpm bug), symlink all headers
mkdir $RPM_BUILD_ROOT%{_includedir}/ncurses{,w}
for l in $RPM_BUILD_ROOT%{_includedir}/*.h; do
ln -s ../$(basename $l) $RPM_BUILD_ROOT%{_includedir}/ncurses
ln -s ../$(basename $l) $RPM_BUILD_ROOT%{_includedir}/ncursesw
done

rm -f $RPM_BUILD_ROOT%{_libdir}/libcurses{,w}.so
echo "INPUT(-lncurses)" > $RPM_BUILD_ROOT%{_libdir}/libcurses.so
echo "INPUT(-lncursesw)" > $RPM_BUILD_ROOT%{_libdir}/libcursesw.so

echo "INPUT(-ltinfo)" > $RPM_BUILD_ROOT%{_libdir}/libtermcap.so

rm -f $RPM_BUILD_ROOT%{_libdir}/terminfo
rm -f $RPM_BUILD_ROOT%{_libdir}/pkgconfig/{*_g,ncurses++*}.pc

rm -f $RPM_BUILD_ROOT%{_libdir}/*.a

mkdir -p %{buildroot}/%{_datadir}/license
cp -f COPYING.GPLv2 %{buildroot}/%{_datadir}/license/%{name}
cp -f COPYING.GPLv2 %{buildroot}/%{_datadir}/license/%{name}-base
cp -f COPYING.GPLv2 %{buildroot}/%{_datadir}/license/%{name}-libs

%post -n ncurses-libs -p /sbin/ldconfig

%postun -n ncurses-libs -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_bindir}/[cirt]*
%{_datadir}/license/%{name}
%manifest ncurses.manifest


%files -n  ncurses-libs
%defattr(-,root,root,-)
%{_libdir}/lib*.so.*
%{_datadir}/license/%{name}-libs

%files term -f terms.term
%defattr(-,root,root,-)

%files base -f terms.base
%defattr(-,root,root,-)
%dir %{_sysconfdir}/terminfo
%{rootdatadir}/terminfo
%{_datadir}/tabset
%dir %{_datadir}/terminfo
%{_datadir}/license/%{name}-base

%files -n ncurses-devel
%defattr(-,root,root,-)
%{_bindir}/ncurses*-config
%{_libdir}/lib*.so
%{_libdir}/pkgconfig/*.pc
%dir %{_includedir}/ncurses
%dir %{_includedir}/ncursesw
%{_includedir}/ncurses/*.h
%{_includedir}/ncursesw/*.h
%{_includedir}/*.h

