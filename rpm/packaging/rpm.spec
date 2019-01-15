# run internal testsuite?
%define release_prefix 4.3
%bcond_without check


Name:           rpm
VCS:            external/rpm#ffd7318edc23db77145257774c704b33d171bde0
Version:        4.9.1
Release:        %{?release_prefix:%{release_prefix}.}5.3.%{?dist}%{!?dist:tizen}
Summary:        The RPM package management system
Url:            http://www.rpm.org/
# Partially GPL/LGPL dual-licensed and some bits with BSD
# SourceLicense: (GPLv2+ and LGPLv2+ with exceptions) and BSD
License:        GPLv2+
Group:          System/Base
Source0:        http://rpm.org/releases/rpm-4.9.x/rpm-%{version}.tar.bz2
Source1:        libsymlink.attr
Source11:       db-4.8.30.tar.gz
Source20:       macros
Source21:       find-docs.sh
Source22:       device-sec-policy
Source1001:     rpm.manifest 

Patch1:         db.diff
# quilt patches start here
Patch3:         rpm-4.5.90-pkgconfig-path.patch
Patch4:         rpm-4.5.90-gstreamer-provides.patch
Patch5:         rpm-4.8.0-tilde.patch
Patch6:         rpm-macros.patch
Patch7:         rpm-4.9.0-tizen-arm.patch
Patch10:        remove-translations.patch
Patch11:        rpm-shorten-changelog.patch
Patch12:        no_rep_autop.diff
Patch13:        finddebuginfo.diff
Patch14:        debugsource-package.diff
Patch15:        debugsubpkg.diff
Patch24:        autodeps.diff
Patch49:        mimetype.diff
Patch52:        firmware.diff
Patch56:        buildidprov.diff
Patch64:        pythondeps.diff
Patch65:        fontprovides.diff
Patch66:        rpm-gst-provides.patch
Patch76:        fileattrs.diff
Patch80:        optflags.patch
Patch81:        build_pack_4.9.1_fix.patch
Patch82:        lib_rpmdb_4.9.1_fix.patch
Patch83:        rpmbuild_4.9.1_fix.patch
Patch84:        rpmbuild_rpmfc_4.9.1_fix.patch
Patch85:        rpmio_base64_4.9.1_fix.patch
Patch86:        rpmlib_format_value_4.9.1_fix.patch
Patch87:        security_4.9.1.patch

##PYTHON##

BuildRequires:  bzip2-devel >= 0.9.0c-2
BuildRequires:  elfutils-devel >= 0.112
BuildRequires:  elfutils-libelf-devel
BuildRequires:  libfile-devel
BuildRequires:  gettext-tools
BuildRequires:  libcap-devel
BuildRequires:  liblua-devel >= 5.1
BuildRequires:  pkgconfig(ncurses)
# The popt version here just documents an older known-good version
BuildRequires:  popt-devel >= 1.10.2
BuildRequires:  readline-devel
BuildRequires:  xz-devel >= 4.999.8
BuildRequires:  zlib-devel
BuildRequires:  nss-devel
BuildRequires:  uthash-devel  
BuildRequires:  libxml2-devel  
BuildRequires:  libattr-devel  
BuildRequires:  pkgconfig(libsmack)


%description
The RPM Package Manager (RPM) is a powerful command line driven
package management system capable of installing, uninstalling,
verifying, querying, and updating software packages. Each software
package consists of an archive of files along with information about
the package like its version, a description, etc.

%package libs
License:        GPLv2+ and LGPLv2+ with exceptions
Summary:        Libraries for manipulating RPM packages
Group:          Development/Libraries
Requires:       rpm = %{version}

%description libs
This package contains the RPM shared libraries.

%package devel
License:        GPLv2+ and LGPLv2+ with exceptions
Summary:        Development files for manipulating RPM packages
Group:          Development/Libraries
Requires:       libfile-devel
Requires:       rpm = %{version}

%description devel
This package contains the RPM C library and header files. These
development files will simplify the process of writing programs that
manipulate RPM packages and databases. These files are intended to
simplify the process of creating graphical package managers or any
other tools that need an intimate knowledge of RPM packages in order
to function.

This package should be installed if you want to develop programs that
will manipulate RPM packages and databases.

%package build
Summary:        Scripts and executable programs used to build packages
Group:          Development/Tools
Requires:       binutils
Requires:       bzip2
Requires:       /bin/cpio
Requires:       diffutils
Requires:       elfutils >= 0.128
Requires:       file
Requires:       /bin/find
Requires:       /bin/awk
Requires:       /bin/grep
Requires:       /bin/gzip
Requires:       lzma
Requires:       patch >= 2.5
Requires:       pkg-config
Requires:       rpm = %{version}
Requires:       /bin/sed
Requires:       unzip
Requires:       xz

%description build
The rpm-build package contains the scripts and executable programs
that are used to build packages using the RPM Package Manager.


%package security-plugin
Summary: MSM security plugin for rpm
Group: Development/Libraries
Requires: rpm = %{version}-%{release}
Requires: libxml2
Requires: file

%description security-plugin
This package contains the MSM security plugin for rpm that performs
security-related functionality. 

%prep
%setup -q  -n rpm-%{version}
tar xzf %{SOURCE11}
ln -s db-4.8.30 db
chmod -R u+w db/*
rm -f rpmdb/db.h

%patch1 -p0
%patch3 -p1 -b .pkgconfig-path
#%patch4 -p1 -b .gstreamer-prov
%patch5 -p1 -b .tilde
%patch6 -p1 -b .vendor
%patch7 -p1 -b .arm
%patch10 -p1
%patch11 -p1
%patch12 -p0
%patch13 -p0
%patch14 -p0
#%patch15 -p0
#%patch24 -p0
#%patch49 -p0
#%patch52 -p0
#%patch56 -p0
#%patch64 -p0
#%patch65 -p0
#%patch66 -p0
#%patch76 -p0
%patch80 -p1
%patch81 -p1
%patch82 -p1
%patch83 -p1
%patch84 -p1
%patch85 -p1
%patch86 -p1
%patch87 -p1 -b .msm

%{?opt_mcpu: %define replace_mopt 1}
%{!?opt_mcpu: %define replace_mopt 0}

%if %{replace_mopt}
sed -e "/^Optflags: %{_target_cpu}/ s/-march[^ ]*/-mcpu=%{opt_mcpu}/g" -i rpmrc.in
sed -e "/^Optflags: %{_target_cpu}/ s/-mtune[^ ]*//g" -i rpmrc.in
sed -e "/^Optflags: %{_target_cpu}/ s/  / /g" -i rpmrc.in
grep "^Optflags: %{_target_cpu}" -rn rpmrc.in
%endif

rm -f m4/libtool.m4
rm -f m4/lt*.m4

%build
cp %{SOURCE1001} .
CPPFLAGS="$CPPFLAGS `pkg-config --cflags nss`"
CFLAGS="%{optflags}"
export CPPFLAGS CFLAGS LDFLAGS

# Using configure macro has some unwanted side-effects on rpm platform
# setup, use the old-fashioned way for now only defining minimal paths.

libtoolize -f -c
./autogen.sh \
%ifarch %arm
    --build=%{_target_cpu}-tizen-linux-gnueabi \
%else
    --build=%{_target_cpu}-tizen-linux-gnu \
%endif
    --prefix=%{_prefix} \
    --sysconfdir=%{_sysconfdir} \
    --localstatedir=%{_localstatedir} \
    --sharedstatedir=%{_localstatedir}/lib \
    --libdir=%{_libdir} \
    --libexecdir=%{_libexecdir} \
%if %{with python}
    --enable-python \
%endif
    --with-vendor=tizen \
    --with-lua \
    --with-cap  \
    --with-msm \
    --disable-dchecks

make %{?_smp_mflags}

%install

%make_install
find %{buildroot} -regex ".*\\.la$" | xargs rm -f --

mkdir -p %{buildroot}%{_sysconfdir}/rpm
mkdir -p %{buildroot}%{_libexecdir}/rpm
mkdir -p %{buildroot}%{_libexecdir}/rpm-plugins
install -m 644 %{SOURCE1} %{buildroot}%{_libexecdir}/rpm/fileattrs/libsymlink.attr
install -m 644 %{SOURCE22} ${RPM_BUILD_ROOT}%{_sysconfdir}/device-sec-policy
install -m 644 %{SOURCE22} ${RPM_BUILD_ROOT}%{_libexecdir}/rpm-plugins/msm-device-sec-policy
mkdir -p %{buildroot}%{_localstatedir}/lib/rpm

#install -m 755 scripts/firmware.prov %{buildroot}%{_prefix}/lib/rpm
#install -m 755 scripts/debuginfo.prov %{buildroot}%{_prefix}/lib/rpm

for dbi in \
    Basenames Conflictname Dirnames Group Installtid Name Packages \
    Providename Provideversion Requirename Requireversion Triggername \
    Filedigests Pubkeys Sha1header Sigmd5 Obsoletename \
    __db.001 __db.002 __db.003 __db.004 __db.005 __db.006 __db.007 \
    __db.008 __db.009
do
    touch %{buildroot}%{_localstatedir}/lib/rpm/$dbi
done

#macros

mkdir -p %{buildroot}%{_libexecdir}/rpm/tizen
install -m 755 %{SOURCE21} %{buildroot}%{_libexecdir}/rpm/tizen
install -m 644 %{SOURCE20} %{buildroot}%{_libexecdir}/rpm/tizen

mkdir -p %{buildroot}/usr/share/license
cp %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/usr/share/license/%{name}
cp %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/usr/share/license/%{name}-libs
cp %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/usr/share/license/%{name}-security-plugin

# avoid dragging in tonne of perl libs for an unused script
chmod 0644 %{buildroot}/%{_libexecdir}/rpm/perldeps.pl

rm -rf %{buildroot}%{_mandir}/*/man?

%remove_docs
rm -rf %{buildroot}/usr/share/man
%find_lang rpm

%if %{with check}
%check
make check
%endif

%clean
rm -rf %{buildroot}

%post libs -p /sbin/ldconfig
%postun libs -p /sbin/ldconfig

%posttrans
# XXX this is klunky and ugly, rpm itself should handle this
dbstat=/usr/lib/rpm/rpmdb_stat
if [ -x "$dbstat" ]; then
    if "$dbstat" -e -h /var/lib/rpm 2>&1 | grep -q "doesn't match environment version \| Invalid argument"; then
        rm -f /var/lib/rpm/__db.*
    fi
fi
exit 0


%files  -f rpm.lang
%defattr(-,root,root,-)
/usr/share/license/rpm
%doc GROUPS COPYING CREDITS
%exclude /usr/lib/rpm/rpmdb_loadcvt
%dir %{_sysconfdir}/rpm

%attr(0755, root, root)   %dir %{_localstatedir}/lib/rpm
%attr(0644, root, root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) %{_localstatedir}/lib/rpm/*
%attr(0755, root, root) %dir %{_libexecdir}/rpm

/bin/rpm
%{_bindir}/rpmkeys
%{_bindir}/rpmspec
%{_bindir}/rpm2cpio
%{_bindir}/rpmdb
%{_bindir}/rpmsign
%{_bindir}/rpmquery
%{_bindir}/rpmverify

%{_libdir}/rpm-plugins/exec.so
%{_libdir}/rpm-plugins/sepolicy.so

%{_libexecdir}/rpm/macros
%{_libexecdir}/rpm/tizen/macros
%{_libexecdir}/rpm/rpmpopt*
%{_libexecdir}/rpm/rpmrc

%{_libexecdir}/rpm/rpmdb_*
%{_libexecdir}/rpm/rpm.daily
%{_libexecdir}/rpm/rpm.log
%{_libexecdir}/rpm/rpm2cpio.sh
%{_libexecdir}/rpm/tgpg

%{_libexecdir}/rpm/platform
%manifest rpm.manifest

%files libs
%defattr(-,root,root)
%{_libdir}/librpm*.so.*
/usr/share/license/%{name}-libs
%manifest rpm.manifest

%files build
%defattr(-,root,root)
%{_bindir}/rpmbuild
%{_bindir}/gendiff

%{_libexecdir}/rpm/fileattrs/*.attr
%{_libexecdir}/rpm/script.req

%{_libexecdir}/rpm/brp-*
%{_libexecdir}/rpm/check-buildroot
%{_libexecdir}/rpm/check-files
%{_libexecdir}/rpm/check-prereqs
%{_libexecdir}/rpm/check-rpaths*
%{_libexecdir}/rpm/debugedit
%{_libexecdir}/rpm/elfdeps
%{_libexecdir}/rpm/find-debuginfo.sh
%{_libexecdir}/rpm/tizen/find-docs.sh
%{_libexecdir}/rpm/find-lang.sh
%{_libexecdir}/rpm/find-provides
%{_libexecdir}/rpm/find-requires
%{_libexecdir}/rpm/javadeps
%{_libexecdir}/rpm/mono-find-provides
%{_libexecdir}/rpm/mono-find-requires
%{_libexecdir}/rpm/ocaml-find-provides.sh
%{_libexecdir}/rpm/ocaml-find-requires.sh
%{_libexecdir}/rpm/osgideps.pl
%{_libexecdir}/rpm/perldeps.pl
%{_libexecdir}/rpm/libtooldeps.sh
%{_libexecdir}/rpm/pkgconfigdeps.sh
%{_libexecdir}/rpm/perl.prov
#%{_libexecdir}/rpm/debuginfo.prov
#%{_libexecdir}/rpm/firmware.prov
%{_libexecdir}/rpm/perl.req
%{_libexecdir}/rpm/tcl.req
%{_libexecdir}/rpm/pythondeps.sh
%{_libexecdir}/rpm/rpmdeps
%{_libexecdir}/rpm/config.guess
%{_libexecdir}/rpm/config.sub
%{_libexecdir}/rpm/mkinstalldirs
%{_libexecdir}/rpm/desktop-file.prov
%{_libexecdir}/rpm/fontconfig.prov

%{_libexecdir}/rpm/macros.perl
%{_libexecdir}/rpm/macros.python
%{_libexecdir}/rpm/macros.php

%manifest rpm.manifest


%files devel
%defattr(-,root,root)
%{_includedir}/rpm
%{_libdir}/librp*[a-z].so
%{_bindir}/rpmgraph
%{_libdir}/pkgconfig/rpm.pc
%manifest rpm.manifest


%files security-plugin
%defattr(-,root,root)
%{_libdir}/rpm-plugins/msm.so
%config(noreplace) %{_sysconfdir}/device-sec-policy
%{_libexecdir}/rpm-plugins/msm-device-sec-policy
/usr/share/license/%{name}-security-plugin
%manifest rpm.manifest
