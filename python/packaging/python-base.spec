Name:           python-base
BuildRequires:  fdupes pkgconfig zlib-devel
Url:            http://www.python.org/
License:        MIT License (or similar)
Group:          Development/Languages/Python
AutoReqProv:    on
# bug437293
Summary:        Python Interpreter base package
Version:        2.7.1
Release:        4
%define         tarversion      %{version}
%define         tarname         Python-%{tarversion}
Source0:        %{tarname}.tar.bz2
Source1:        macros.python
Source2:        baselibs.conf
Source3:        README.SUSE
Source4:        distutils.cfg
Source5:        _local.pth

Patch1:         python-2.7-dirs.patch
Patch2:         python-distutils-rpm-8.patch
Patch3:         python-2.7.1-multilib.patch
Patch4:         python-2.7rc2-canonicalize2.patch
Patch5:         python-2.7rc2-configure.patch
Patch6:         python-2.6-gettext-plurals.patch
Patch7:         python-2.7.1-distutils_test_path.patch
Patch8:         sparc_longdouble.patch
Patch12:        http://psf.upfronthosting.co.za/roundup/tracker/file19029/python-test_structmembers.patch
Patch13:        python-2.7.1-fix_date_time_compiler.patch
Patch14:        python-2.7-CVE-2011-1521-fileurl.patch
Patch15:        python-2.7-fix-parallel-make.patch
Patch16:        python-2.7.1-linux3.patch
Patch17:        subprocess-can-raise-OSError-EPIPE-when-communicatin.patch
Patch100:       python-2.7.1-home64.patch
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
%define         python_version    %(echo %{version} | head -c 3)
Provides:       %{name} = %{python_version}

# explicitly, see bnc#697251
Requires:       libpython = %{version}-%{release}

%description
Python is an interpreted, object-oriented programming language, and is
often compared to Tcl, Perl, Scheme, or Java.  You can find an overview
of Python in the documentation and tutorials included in the python-doc
(HTML) or python-doc-pdf (PDF) packages.

This package contains all of stand-alone Python files, minus binary
modules that would pull in extra dependencies.



Authors:
--------
    Guido van Rossum <guido@python.org>

%package -n python-devel
License:        Python License ..
Requires:       python-base = %{version} glibc-devel
Summary:        Include Files and Libraries Mandatory for Building Python Modules
Group:          Development/Languages/Python
AutoReqProv:    on

%description -n python-devel
The Python programming language's interpreter can be extended with
dynamically loaded extensions and can be embedded in other programs.

This package contains header files, a static library, and development
tools for building Python modules, extending the Python interpreter or
embedding Python in applications.



Authors:
--------
    Guido van Rossum <guido@python.org>

%package -n python-xml
License:        Python License ..
Requires:       python-base = %{version}
Summary:        A Python XML Interface
Group:          Development/Libraries/Python
# pyxml used to live out of tree
Provides:       pyxml = 0.8.5
Obsoletes:      pyxml < 0.8.5
AutoReqProv:    on

%description -n python-xml
The expat module is a Python interface to the expat XML parser. Since
Python2.x, it is part of the core Python distribution.



%package -n libpython
License:        MIT License (or similar)
Summary:        Python Interpreter shared library
Group:          Development/Languages/Python
AutoReqProv:    on

%description -n libpython
Python is an interpreted, object-oriented programming language, and is
often compared to Tcl, Perl, Scheme, or Java.  You can find an overview
of Python in the documentation and tutorials included in the python-doc
(HTML) or python-doc-pdf (PDF) packages.

This package contains libpython2.6 shared library for embedding in
other applications.



Authors:
--------
    Guido van Rossum <guido@python.org>

%prep
%setup -q -n %{tarname}
# patching
%patch1 -p1
%patch2 -p1
#patch3 -p1
%patch4
%patch5
%patch6
%patch7 -p1
%patch8 -p1
%patch12
%patch13 -p1
%patch14 -p1
%patch15 -p1
%patch16 -p1
%patch17 -p1
%patch100 -p1

# drop Autoconf version requirement
sed -i 's/^version_required/dnl version_required/' configure.in

%build
export OPT="$RPM_OPT_FLAGS"

autoreconf -f -i . # Modules/_ctypes/libffi

# provide a stable timestamp
touch -r %{S:0} Makefile.pre.in

# Cheat for kernel 3.0
cp -r Lib/plat-linux2 Lib/plat-linux3

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

LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH \
    make %{?_smp_mflags} \
        DESTDIR=$RPM_BUILD_ROOT \
        profile-opt


%install
# replace rest of /usr/local/bin/python or /usr/bin/python2.5 with /usr/bin/python
find . -wholename "./Parser" -prune -o -name '*.py' -type f -print0 | xargs -0 grep -lE '^#! *(/usr/.*bin/(env +)?)?python' | xargs sed -r -i -e '1s@^#![[:space:]]*(/usr/(local/)?bin/(env +)?)?python([0-9]+\.[0-9]+)?@#!/usr/bin/python@'
# the grep inbetween makes it much faster
########################################
# install it
########################################
make \
    OPT="$RPM_OPT_FLAGS -fPIC" \
    DESTDIR=$RPM_BUILD_ROOT \
    install
# install site-specific tweaks
ln -s python%{python_version} ${RPM_BUILD_ROOT}%{_bindir}/python2
install -m 644 %{S:4} ${RPM_BUILD_ROOT}%{_prefix}/lib/python%{python_version}/distutils
install -m 644 %{S:5} ${RPM_BUILD_ROOT}%{_prefix}/lib/python%{python_version}/site-packages
install -d -m 755 ${RPM_BUILD_ROOT}/etc/rpm
install -m 644 %{S:1} ${RPM_BUILD_ROOT}/etc/rpm
# make sure /usr/lib/python/site-packages exists even on lib64 machines
mkdir -p ${RPM_BUILD_ROOT}/usr/lib/python%{python_version}/site-packages
########################################
# some cleanups
########################################
# remove hard links and replace them with symlinks
for dir in bin include lib ; do
    rm -f $RPM_BUILD_ROOT/%{_prefix}/$dir/python
    ln -s python%{python_version} $RPM_BUILD_ROOT/%{_prefix}/$dir/python
done
CLEANUP_DIR="$RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}"
# don't distribute precompiled windows installers (duh)
rm -f $CLEANUP_DIR/distutils/command/*.exe
# kill imageop.so and audioop.so, they are rarely used and insecure
rm -f $CLEANUP_DIR/lib-dynload/imageop.so
rm -f $CLEANUP_DIR/lib-dynload/audioop.so
# remove various things that don't need to be in python-base
rm $RPM_BUILD_ROOT%{_bindir}/idle
rm -r $CLEANUP_DIR/{curses,bsddb,idlelib,lib-tk,sqlite3}
rm $CLEANUP_DIR/ssl.py*

# remove all pre-compiled pyc, pyo files to resolve conflict of *-x86-arm packages
find $CLEANUP_DIR -name '*.py[co]' -print | xargs rm -f
#        does not work without _ssl.so anyway
# replace duplicate .pyo/.pyc with hardlinks
%fdupes $RPM_BUILD_ROOT/%{_prefix}/lib/python%{python_version}
########################################
# documentation
########################################
export PDOCS=${RPM_BUILD_ROOT}%{_docdir}/%{name}
install -d -m 755 $PDOCS
install -c -m 644 %{S:3}                        $PDOCS/
install -c -m 644 LICENSE                           $PDOCS/
install -c -m 644 README                            $PDOCS/
ln -s python%{python_version}.1.gz ${RPM_BUILD_ROOT}%{_mandir}/man1/python.1.gz
########################################
# devel
########################################
# install Makefile.pre.in and Makefile.pre
cp Makefile Makefile.pre.in Makefile.pre $RPM_BUILD_ROOT%{_prefix}/lib/python%{python_version}/config/

mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/python-devel
cp LICENSE %{buildroot}/usr/share/license/python-xml
cp LICENSE %{buildroot}/usr/share/license/libpython
cp LICENSE %{buildroot}/usr/share/license/%{name}

%clean
rm -rf $RPM_BUILD_ROOT

%post -n libpython
/sbin/ldconfig

%postun -n libpython
/sbin/ldconfig

%files -n python-devel
%defattr(-, root, root)
%{_prefix}/lib/python%{python_version}/config/*
%exclude %{_prefix}/lib/python%{python_version}/config/Setup
%exclude %{_prefix}/lib/python%{python_version}/config/Makefile
%defattr(644, root, root, 755)
%{_libdir}/libpython*.so
%{_libdir}/pkgconfig/python-%{python_version}.pc
%{_libdir}/pkgconfig/python.pc
%{_includedir}/python*
%exclude %{_includedir}/python%{python_version}/pyconfig.h
%{_prefix}/lib/python%{python_version}/test
%defattr(755, root, root)
%{_bindir}/python-config
%{_bindir}/python%{python_version}-config
/usr/share/license/python-devel

%files -n python-xml
%defattr(644, root, root, 755)
%{_prefix}/lib/python%{python_version}/xml
%{_prefix}/lib/python%{python_version}/lib-dynload/pyexpat.so
#%files -n libpython%{lib_version}
/usr/share/license/python-xml

%files -n libpython
%defattr(644, root, root)
%{_libdir}/libpython*.so.*
/usr/share/license/libpython

%files
%defattr(644, root, root, 755)
%config /etc/rpm/macros.python
%dir %{_docdir}/%{name}
%doc %{_docdir}/%{name}/README
%doc %{_docdir}/%{name}/LICENSE
%doc %{_docdir}/%{name}/README.SUSE
%doc %{_mandir}/man1/python.1*
%doc %{_mandir}/man1/python%{python_version}.1*
%dir %{_includedir}/python%{python_version}
%{_includedir}/python%{python_version}/pyconfig.h
%{_prefix}/lib/python
%dir /usr/lib/python%{python_version}
%dir /usr/lib/python%{python_version}/site-packages
%dir %{_prefix}/lib/python%{python_version}
%dir %{_prefix}/lib/python%{python_version}/config
%{_prefix}/lib/python%{python_version}/config/Setup
%{_prefix}/lib/python%{python_version}/config/Makefile
%{_prefix}/lib/python%{python_version}/*.*
%{_prefix}/lib/python%{python_version}/compiler
%{_prefix}/lib/python%{python_version}/ctypes
%{_prefix}/lib/python%{python_version}/distutils
%{_prefix}/lib/python%{python_version}/email
%{_prefix}/lib/python%{python_version}/encodings
%{_prefix}/lib/python%{python_version}/hotshot
%{_prefix}/lib/python%{python_version}/importlib
%{_prefix}/lib/python%{python_version}/json
%{_prefix}/lib/python%{python_version}/lib2to3
%{_prefix}/lib/python%{python_version}/logging
%{_prefix}/lib/python%{python_version}/multiprocessing
%{_prefix}/lib/python%{python_version}/plat-*
%{_prefix}/lib/python%{python_version}/pydoc_data
%{_prefix}/lib/python%{python_version}/unittest
%{_prefix}/lib/python%{python_version}/wsgiref
%dir %{_prefix}/lib/python%{python_version}/site-packages
%{_prefix}/lib/python%{python_version}/site-packages/README
%{_prefix}/lib/python%{python_version}/site-packages/_local.pth
%dir %{_prefix}/lib/python%{python_version}/lib-dynload
%{_prefix}/lib/python%{python_version}/lib-dynload/_bisect.so
#%{_prefix}/lib/python%{python_version}/lib-dynload/_bytesio.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_csv.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_collections.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_ctypes.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_ctypes_test.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_elementtree.so
#%{_prefix}/lib/python%{python_version}/lib-dynload/_fileio.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_functools.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_heapq.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_hotshot.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_io.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_json.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_locale.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_lsprof.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_md5.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_multiprocessing.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_random.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_sha.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_sha256.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_sha512.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_socket.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_struct.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_testcapi.so
%{_prefix}/lib/python%{python_version}/lib-dynload/array.so
%{_prefix}/lib/python%{python_version}/lib-dynload/binascii.so
#arm doesn't build this module
%ifnarch %arm
##{_libdir}/python%{python_version}/lib-dynload/bz2.so
%endif
%{_prefix}/lib/python%{python_version}/lib-dynload/cPickle.so
%{_prefix}/lib/python%{python_version}/lib-dynload/cStringIO.so
%{_prefix}/lib/python%{python_version}/lib-dynload/cmath.so
%{_prefix}/lib/python%{python_version}/lib-dynload/crypt.so
%{_prefix}/lib/python%{python_version}/lib-dynload/datetime.so
%{_prefix}/lib/python%{python_version}/lib-dynload/fcntl.so
%{_prefix}/lib/python%{python_version}/lib-dynload/future_builtins.so
%{_prefix}/lib/python%{python_version}/lib-dynload/grp.so
%{_prefix}/lib/python%{python_version}/lib-dynload/itertools.so
%{_prefix}/lib/python%{python_version}/lib-dynload/linuxaudiodev.so
%{_prefix}/lib/python%{python_version}/lib-dynload/math.so
%{_prefix}/lib/python%{python_version}/lib-dynload/mmap.so
%{_prefix}/lib/python%{python_version}/lib-dynload/nis.so
%{_prefix}/lib/python%{python_version}/lib-dynload/operator.so
%{_prefix}/lib/python%{python_version}/lib-dynload/ossaudiodev.so
%{_prefix}/lib/python%{python_version}/lib-dynload/parser.so
%{_prefix}/lib/python%{python_version}/lib-dynload/resource.so
%{_prefix}/lib/python%{python_version}/lib-dynload/select.so
%{_prefix}/lib/python%{python_version}/lib-dynload/spwd.so
%{_prefix}/lib/python%{python_version}/lib-dynload/strop.so
%{_prefix}/lib/python%{python_version}/lib-dynload/syslog.so
%{_prefix}/lib/python%{python_version}/lib-dynload/termios.so
%{_prefix}/lib/python%{python_version}/lib-dynload/time.so
%{_prefix}/lib/python%{python_version}/lib-dynload/unicodedata.so
%{_prefix}/lib/python%{python_version}/lib-dynload/zlib.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_codecs*.so
%{_prefix}/lib/python%{python_version}/lib-dynload/_multibytecodec.so
%{_prefix}/lib/python%{python_version}/lib-dynload/Python-%{tarversion}-py%{python_version}.egg-info
# these modules don't support 64-bit arches (disabled by setup.py)
%ifnarch alpha ia64 x86_64 s390x ppc64 sparc64
# requires sizeof(int) == sizeof(long) == sizeof(char*)
%{_prefix}/lib/python%{python_version}/lib-dynload/dl.so
%endif
%attr(755, root, root) %{_bindir}/pydoc
%attr(755, root, root) %{_bindir}/python
%attr(755, root, root) %{_bindir}/2to3
%attr(755, root, root) %{_bindir}/python%{python_version}
%attr(755, root, root) %{_bindir}/smtpd.py
%{_bindir}/python2
/usr/share/license/%{name}

%changelog
