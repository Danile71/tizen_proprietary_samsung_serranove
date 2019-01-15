Name:           python
Url:            http://www.python.org/
License:        MIT License (or similar)
Group:          Development/Languages/Python
Obsoletes:      python-nothreads python21 python-elementtree python-sqlite
Summary:        Python Interpreter
Version:        2.7.1
Release:        1
Requires:       python-base = %{version}
BuildRequires:  db4-devel
BuildRequires:  fdupes
BuildRequires:  gdbm-devel
BuildRequires:  bzip2-devel
BuildRequires:  openssl-devel
BuildRequires:  ncurses-devel
BuildRequires:  readline-devel
BuildRequires:  sqlite-devel
BuildRequires:  zlib-devel

%define         tarversion      %{version}
%define         tarname         Python-%{tarversion}
Source0:        %{tarname}.tar.bz2
Source2:        pythonstart
Source3:        python.sh
Source4:        python.csh
#Source11:       testfiles.tar.bz2
# issues with copyrighted Unicode testing files
Patch1:         python-2.7-dirs.patch
Patch2:         python-2.7.1-multilib.patch
Patch3:         python-2.7rc2-canonicalize2.patch
Patch4:         python-2.5.1-sqlite.patch
Patch5:         python-2.7rc2-configure.patch
Patch6:         python-2.6b3-curses-panel.patch
Patch7:         sparc_longdouble.patch
Patch9:         python-2.7.1-fix_date_time_compiler.patch
Patch10:        python-2.7-fix-parallel-make.patch
Patch11:        python-2.7.1-linux3.patch
Patch100:        python-2.7.1-home64.patch

%define         python_version    %(echo %{version} | head -c 3)
%define         idle_name         idle
Provides:       %{name} = %{python_version}

%description
Python is an interpreted, object-oriented programming language, and is
often compared to Tcl, Perl, Scheme, or Java.  You can find an overview
of Python in the documentation and tutorials included in the python-doc
(HTML) or python-doc-pdf (PDF) packages.

If you want to install third party modules using distutils, you need to
install python-devel package.



Authors:
--------
    Guido van Rossum <guido@python.org>

%package idle
License:        Python License ..
Requires:       python-base = %{version}
Summary:        An Integrated Development Environment for Python
Group:          Development/Languages/Python

%description idle
IDLE is a Tkinter based integrated development environment for Python.
It features a multi-window text editor with multiple undo, Python
colorizing, and many other things, as well as a Python shell window and
a debugger.


Authors:
--------
    Guido van Rossum <guido@python.org>

%package demo
License:        Python License ..
Provides:       pyth_dmo
Obsoletes:      pyth_dmo
Requires:       python-base = %{version}
Summary:        Python Demonstration Scripts
Group:          Development/Languages/Python

%description demo
Various demonstrations of what you can do with Python and a number of
programs that are useful for building or extending Python.



Authors:
--------
    Guido van Rossum <guido@python.org>


%package curses
License:        Python License ..
Requires:       python-base = %{version}
Obsoletes:      pyth_cur
Provides:       pyth_cur
Summary:        Python Interface to the (N)Curses Library
Group:          Development/Libraries/Python

%description curses
An easy to use interface to the (n)curses CUI library. CUI stands for
Console User Interface.



Authors:
--------
    Guido van Rossum <guido@python.org>

%package gdbm
License:        MIT License (or similar)
Requires:       python-base = %{version}
Obsoletes:      pygdmod
Provides:       pygdmod
Summary:        Python Interface to the GDBM Library
Group:          Development/Libraries/Python

%description gdbm
An easy to use interface for GDBM databases. GDBM is the GNU
implementation of the standard Unix DBM databases.



Authors:
--------
    Guido van Rossum <guido@python.org>

%prep
%setup -q -n %{tarname}
# patching
%patch1 -p1
#patch2 -p1
%patch3
%patch4
%patch5
%patch6
%patch7 -p1
%patch9 -p1
%patch10 -p1
%patch11 -p1
%patch100 -p1

# drop Autoconf version requirement
sed -i 's/^version_required/dnl version_required/' configure.in

%build
export OPT="$RPM_OPT_FLAGS"

autoreconf -f -i . # Modules/_ctypes/libffi
# prevent make from trying to rebuild asdl stuff, which requires existing
# python installation
touch Parser/asdl* Python/Python-ast.c Include/Python-ast.h

./configure \
    --prefix=%{_prefix} \
    --libdir=%{_libdir} \
    --mandir=%{_mandir} \
    --docdir=%{_docdir}/python \
    --enable-ipv6 \
    --with-fpectl \
    --enable-shared \
    --enable-unicode=ucs4

make %{?_smp_mflags} DESTDIR=$RPM_BUILD_ROOT

#%check
# on hppa, the threading of glibc is quite broken. The tests just stop
# at some point, and the machine does not build anything more until a
# timeout several hours later.
#%ifnarch  %arm
# Limit virtual memory to avoid spurious failures
#if test $(ulimit -v) = unlimited || test $(ulimit -v) -gt 10000000; then
#  ulimit -v 10000000 || :
#fi
#LIST="test_urllib test_ssl test_hashlib test_hmac test_urllib2_localnet test_unicodedata test_tarfile test_sqlite test_tcl test_anydbm test_dumbdbm test_gdbm test_whichdb test_tk test_ttk_textonly test_bsddb test_readline "
#make test TESTOPTS="$LIST"
#%endif

%install
# replace rest of /usr/local/bin/python or /usr/bin/python2.x with /usr/bin/python
find . -wholename "./Parser" -prune -o -name '*.py' -type f -print0 | xargs -0 grep -lE '^#! *(/usr/.*bin/(env +)?)?python' | xargs sed -r -i -e '1s@^#![[:space:]]*(/usr/(local/)?bin/(env +)?)?python([0-9]+\.[0-9]+)?@#!/usr/bin/python@'
# the grep inbetween makes it much faster
########################################
# install it
########################################
make \
    OPT="$RPM_OPT_FLAGS -fPIC" \
    DESTDIR=$RPM_BUILD_ROOT \
    install
########################################
# some cleanups
########################################
# remove hard links and replace them with symlinks
for dir in bin include lib ; do
    rm -f $RPM_BUILD_ROOT/%{_prefix}/$dir/python
    ln -s python%{python_version} $RPM_BUILD_ROOT/%{_prefix}/$dir/python
done
# kill imageop.so, it's insecure
rm -f $RPM_BUILD_ROOT/%{_prefix}/lib/python%{python_version}/lib-dynload/imageop.so
#cleanup for -base
rm -rf $RPM_BUILD_ROOT/%{_prefix}/lib/python%{python_version}/lib-tk
rm $RPM_BUILD_ROOT%{_bindir}/python{,%{python_version}}
rm $RPM_BUILD_ROOT%{_bindir}/smtpd.py
rm $RPM_BUILD_ROOT%{_bindir}/pydoc
rm $RPM_BUILD_ROOT%{_bindir}/2to3
rm $RPM_BUILD_ROOT%{_mandir}/man1/python*
rm $RPM_BUILD_ROOT%{_libdir}/libpython*.so.*
rm $RPM_BUILD_ROOT%{_prefix}/lib/python
find $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version} -maxdepth 1 ! \( -name "ssl.py" \) -exec rm {} ";"
rm $RPM_BUILD_ROOT%{_bindir}/python%{python_version}-config
rm $RPM_BUILD_ROOT%{_bindir}/python-config
rm $RPM_BUILD_ROOT%{_libdir}/pkgconfig/*
rm -r $RPM_BUILD_ROOT%{_includedir}/python
rm -r $RPM_BUILD_ROOT%{_includedir}/python%{python_version}
rm -r $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/compiler
rm -r $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/config
rm -r $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/ctypes
rm -r $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/distutils
rm -r $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/email
rm -r $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/encodings
rm -r $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/hotshot
rm -r $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/importlib
rm -r $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/json
rm -r $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib2to3
rm -r $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/logging
rm -r $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/multiprocessing
rm -r $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/plat-*
rm -r $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/pydoc_data
rm -r $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/test
rm -r $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/unittest
rm -r $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/wsgiref
rm $RPM_BUILD_ROOT%{_libdir}/libpython%{python_version}.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/site-packages/README
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/_bisect.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/_csv.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/_collections.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/_ctypes.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/_ctypes_test.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/_elementtree.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/_functools.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/_heapq.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/_hotshot.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/_io.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/_json.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/_locale.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/_lsprof.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/_multiprocessing.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/_random.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/_socket.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/_struct.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/_testcapi.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/array.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/binascii.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/cPickle.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/cStringIO.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/cmath.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/crypt.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/datetime.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/fcntl.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/future_builtins.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/grp.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/itertools.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/linuxaudiodev.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/math.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/mmap.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/nis.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/operator.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/ossaudiodev.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/parser.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/resource.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/select.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/spwd.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/strop.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/syslog.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/termios.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/time.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/unicodedata.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/zlib.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/_codecs*.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/_multibytecodec.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/audioop.so
rm -f $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/dl.so
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/Python-%{tarversion}-py%{python_version}.egg-info
rm $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/lib-dynload/pyexpat.so
rm -r $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/xml/*



# remove all pre-compiled pyc, pyo files to resolve conflict of *-x86-arm packages
find $RPM_BUILD_ROOT/%{_prefix}/lib/python%{python_version} -name '*.py[co]' -print | xargs rm -f


export PDOCS=${RPM_BUILD_ROOT}%{_docdir}/%{name}
install -d -m 755 $PDOCS

########################################
# tools and demos
########################################
find Tools/ Demo/ -type d \( -regex ".*/.cvsignore" \) -exec rm -f \{\} \;
for x in `find Tools/ Demo/ \( -not -name Makefile \) -print | sort` ; do
  test -d $x && ( install -c -m 755 -d $PDOCS/$x ) \
             || ( install -c -m 644 $x $PDOCS/$x )
done
########################################
# idle
########################################
# move idle config into /etc
install -d -m755 ${RPM_BUILD_ROOT}/etc/%{idle_name}
(
    cd ${RPM_BUILD_ROOT}/%{_prefix}/lib/python%{python_version}/idlelib/
    for file in *.def ; do
        mv $file ${RPM_BUILD_ROOT}/etc/%{idle_name}/
        ln -sf /etc/%{idle_name}/$file  ${RPM_BUILD_ROOT}/%{_prefix}/lib/python%{python_version}/idlelib/
    done
)
########################################
# startup script
########################################
install -m 644 %{S:2} $RPM_BUILD_ROOT/etc
install -d -m 755 $RPM_BUILD_ROOT/etc/profile.d
install -m 644 %{S:3} %{S:4} $RPM_BUILD_ROOT/etc/profile.d

mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}-idle
cp LICENSE %{buildroot}/usr/share/license/%{name}-demo
cp LICENSE %{buildroot}/usr/share/license/%{name}-curses
cp LICENSE %{buildroot}/usr/share/license/%{name}-gdbm
cp LICENSE %{buildroot}/usr/share/license/%{name}


%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files idle
%defattr(644, root, root, 755)
%dir /etc/%{idle_name}
%config /etc/%{idle_name}/*
%doc Lib/idlelib/NEWS.txt
%doc Lib/idlelib/README.txt
%doc Lib/idlelib/TODO.txt
%doc Lib/idlelib/extend.txt
%doc Lib/idlelib/ChangeLog
%{_prefix}/lib/python%{python_version}/idlelib
%attr(755, root, root) %{_bindir}/%{idle_name}
/usr/share/license/%{name}-idle

%files demo
%defattr(644, root, root, 755)
%doc %{_docdir}/%{name}/Demo
%doc %{_docdir}/%{name}/Tools
/usr/share/license/%{name}-demo


%files curses
%defattr(644, root, root, 755)
%{_prefix}/lib/python%{python_version}/curses
%{_prefix}/lib/python%{python_version}/lib-dynload/_curses.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_curses_panel.so
/usr/share/license/%{name}-curses

%files gdbm
%defattr(644, root, root, 755)
%{_prefix}/lib/python%{python_version}/lib-dynload/gdbm.so
%{_prefix}/lib/python%{python_version}/lib-dynload/dbm.so
/usr/share/license/%{name}-gdbm

%files
%defattr(644, root, root, 755)
%config /etc/pythonstart
%config /etc/profile.d/python.*
%dir %{_prefix}/lib/python%{python_version}
%{_prefix}/lib/python%{python_version}/ssl.py*
%{_prefix}/lib/python%{python_version}/bsddb
%{_prefix}/lib/python%{python_version}/sqlite3
%dir %{_prefix}/lib/python%{python_version}/lib-dynload
%{_prefix}/lib/python%{python_version}/lib-dynload/_bsddb.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_hashlib.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_sqlite3.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_ssl.so
%{_prefix}/lib/python%{python_version}/lib-dynload/bz2.so
%{_prefix}/lib/python%{python_version}/lib-dynload/readline.so
/usr/share/license/%{name}
