%define multiarcharches %{ix86} x86_64
%define run_glibc_tests 0
%define localepackage 1
%define linaro_release 2014.11
%define release_prefix %{linaro_release}

Summary:a variant of the GNU C Library targetting embedded systems.
Name: 	glibc
Version:2.20
Release:%{release_prefix}
# GPLv2+ is used in a bunch of programs, LGPLv2+ is used for libraries.
# Things that are linked directly into dynamically linked programs
# and shared libraries (e.g. crt files, lib*_nonshared.a) have an additional
# exception which allows linking it into any kind of programs or shared
# libraries without restrictions.
License:LGPLv2+ and LGPLv2+ with exceptions and GPLv2+
Group: 	System/Libraries
URL: 	http://releases.linaro.org/14.19/components/toolchain/glibc-linaro
Source0: glibc-linaro-%{version}-%{linaro_release}.tar.xz
Source5: nsswitch.conf
Source13: generate-supported.mk
Source1001: glibc.manifest

Provides: ldconfig
# The dynamic linker supports DT_GNU_HASH
Provides: rtld(GNU_HASH)
Provides: eglibc
# Workaround hard float ld-linux
#ifarch armv8el armv7hl
#Provides: ld-linux.so.3 ld-linux.so.3(GLIBC_2.4)
#endif
Requires: eglibc-common = %{version}-%{release}
# Require libgcc in case some program calls pthread_cancel in its %%post
Requires(pre): libgcc
# This is for building auxiliary programs like memusage, nscd
# For initial glibc bootstraps it can be commented out
BuildRequires: zlib-devel
BuildRequires: libcap-devel
BuildRequires: gettext-tools >= 0.10.36
# This is to ensure that __frame_state_for is exported by glibc
# will be compatible with egcs 1.x.y
BuildRequires: gcc >= 3.2
# Need AS_NEEDED directive
# Need --hash-style=* support
BuildRequires: binutils >= 2.19.51.0.10
BuildRequires: elfutils >= 0.72
BuildRequires: rpm >= 4.2-0.56
BuildRequires: tzdata

%define enablekernel 2.6.25
%ifarch %{ix86}
%define _target_cpu	i686
%endif
%ifarch i386
%define nptl_target_cpu i486
%else
%define nptl_target_cpu %{_target_cpu}
%endif

%define __find_provides %{_builddir}/%{name}-%{version}/find_provides.sh
%define _filter_GLIBC_PRIVATE 1

%description
Embedded GLIBC (EGLIBC) is a variant of the GNU C Library (GLIBC)  that is designed to work well on embedded systems. EGLIBC strives to be source and binary compatible with GLIBC. EGLIBC's goals include reduced footprint, configurable components, better support for cross-compilation and cross-testing.

%package devel
Summary: Object files for development using standard C libraries
Group: Development/Libraries
#change back after booting with eglibc
#Requires(pre): %{name}-headers
#Requires: %{name}-headers = %{version}-%{release}
Requires(pre): glibc-headers
Requires: glibc-headers = %{version}-%{release}
Requires: %{name} = %{version}-%{release}
Provides: eglibc-devel

%description devel
The glibc-devel package contains the object files necessary
for developing programs which use the standard C libraries (which are
used by nearly all programs).  If you are developing programs which
will use the standard C libraries, your system needs to have these
standard object files available in order to create the
executables.

Install glibc-devel if you are going to develop programs which will
use the standard C libraries.

%package static
Summary: C library static libraries for -static linking
Group: Development/Libraries
Requires: %{name}-devel = %{version}-%{release}
Provides: eglibc-static

%description static
The glibc-static package contains the C library static libraries
for -static linking.  You don't need these, unless you link statically,
which is highly discouraged.

%package headers
Summary: Header files for development using standard C libraries
Group: Development/Libraries
Provides: %{name}-headers(%{_target_cpu})
Provides: eglibc-headers(%{_target_cpu})
Provides: eglibc-headers
Obsoletes: eglibc-headers(i586)
Obsoletes: eglibc-headers(i686)
%ifarch x86_64
# If both -m32 and -m64 is to be supported on AMD64, x86_64 glibc-headers
# have to be installed, not i586 ones.
Obsoletes: %{name}-headers(i586)
Obsoletes: %{name}-headers(i686)
%endif
Requires: %{name} = %{version}-%{release}

%description headers
The glibc-headers package contains the header files necessary
for developing programs which use the standard C libraries (which are
used by nearly all programs).  If you are developing programs which
will use the standard C libraries, your system needs to have these
standard header files available in order to create the
executables.

Install glibc-headers if you are going to develop programs which will
use the standard C libraries.

%package common
Summary: Common binaries and locale data for glibc
Requires: %{name} = %{version}-%{release}
Requires: tzdata >= 2003a
Requires: grep
Requires: findutils
Requires: coreutils
Group: System/Base
Provides: eglibc-common

%description common
The glibc-common package includes common binaries for the GNU libc
libraries, as well as national language (locale) support.

%package -n nscd
Summary: A Name Service Caching Daemon (nscd)
Group: System/Daemons
Requires(pre): /sbin/chkconfig, /usr/sbin/useradd, /usr/sbin/userdel
Provides: eglibc-nscd

%description -n nscd
Nscd caches name service lookups and can dramatically improve
performance with NIS+, and may help with DNS as well.

%package utils
Summary: Development utilities from GNU C library
Group: Development/Tools
Requires: %{name} = %{version}-%{release}
Provides: eglibc-utils

%description utils
The glibc-utils package contains memusage, a memory usage profiler,
mtrace, a memory leak tracer and xtrace, a function call tracer
which can be helpful during program debugging.

If unsure if you need this, don't install this package.


%prep
%setup -q -n glibc-linaro-%{version}-%{linaro_release}

cp %{SOURCE1001} .

cat > find_provides.sh <<EOF
#!/bin/sh
/usr/lib/rpm/find-provides | grep -v GLIBC_PRIVATE
exit 0
EOF
chmod +x find_provides.sh
touch `find . -name configure`
touch locale/programs/*-kw.h

%build
GCC=gcc
GXX=g++
# Fail to build with system flags.syslog.o: inline failed.
#BuildFlags="$RPM_OPT_FLAGS"
BuildFlags="-O2 -g"
echo %{ix86}
%ifarch %{ix86}
BuildFlags="$BuildFlags -march=core2 -mtune=atom"
%endif

%ifnarch %{arm}
BuildFlags="$BuildFlags -fasynchronous-unwind-tables"
%endif

EnableKernel="--enable-kernel=%{enablekernel}"
echo "$GCC" > Gcc
AddOns=`echo */configure | sed -e 's!/configure!!g;s!\(linuxthreads\|nptl\|rtkaio\|powerpc-cpu\)\( \|$\)!!g;s! \+$!!;s! !,!g;s!^!,!;/^,\*$/d'`

%ifarch %{arm}
AddOns=,$AddOns
%endif

build_nptl()
{
builddir=build-%{nptl_target_cpu}-$1
shift
rm -rf $builddir
mkdir $builddir ; cd $builddir
../configure CC="$GCC" CXX="$GXX" CFLAGS="$BuildFlags" \
	--prefix=%{_prefix} \
	--bindir=%{_bindir} \
	--libdir=%{_libdir} \
	--libexecdir=%{_libexecdir} \
	--enable-add-ons=$AddOns --without-cvs $EnableKernel \
	--with-headers=%{_prefix}/include \
	--with-tls --with-__thread  \
%ifnarch %{arm}
	--build %{nptl_target_cpu}-tizen-linux \
	--host %{nptl_target_cpu}-tizen-linux \
%else
	--build %{nptl_target_cpu}-tizen-linux-gnueabi \
	--host %{nptl_target_cpu}-tizen-linux-gnueabi \
%endif
%ifnarch %{multiarcharches}
	--disable-multi-arch \
%endif
	--enable-profile \
	--without-selinux \
	--enable-obsolete-rpc \
	--disable-force-install libc_cv_ssp=no \
	libc_cv_forced_unwind=yes \
	--enable-stackguard-randomization
make %{?_smp_mflags} -r CFLAGS="$build_CFLAGS" PARALLELMFLAGS=-s

cd ..
}

build_nptl linuxnptl

%install
GCC=`cat Gcc`

rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT

mkdir -p %{buildroot}/usr/share/license
cp COPYING %{buildroot}/usr/share/license/%{name}

make install_root=$RPM_BUILD_ROOT install -C build-%{nptl_target_cpu}-linuxnptl PARALLELMFLAGS=-s
#%ifnarch %{auxarches}
%if 0%{localepackage}
mkdir -p %{buildroot}/usr/lib/locale
cd build-%{nptl_target_cpu}-linuxnptl && \
#  make %{?_smp_mflags} install_root=$RPM_BUILD_ROOT install-locales -C ../localedata objdir=`pwd` && \
  I18NPATH=../localedata GCONV_PATH=../iconvdata localedef --quiet -c -f UTF-8 -i POSIX %{buildroot}/usr/lib/locale/C.UTF-8
  cd ..

  make -f %{SOURCE13} IN=localedata/SUPPORTED \
                OUT=%{buildroot}/usr/share/i18n/SUPPORTED;
%endif

# remove /usr/share/i18n unnecessary on the target device
if [ -d %{buildroot}/usr/share/i18n/ ]; then
  rm -rf %{buildroot}/usr/share/i18n/
fi

librtso=`basename $RPM_BUILD_ROOT/%{_lib}/librt.so.*`


# Remove the files we don't want to distribute
rm -f $RPM_BUILD_ROOT%{_prefix}/%{_lib}/libNoVersion*
rm -f $RPM_BUILD_ROOT/%{_lib}/libNoVersion*

# NPTL <bits/stdio-lock.h> is not usable outside of glibc, so include
# the generic one (#162634)
cp -a bits/stdio-lock.h $RPM_BUILD_ROOT%{_prefix}/include/bits/stdio-lock.h
# And <bits/libc-lock.h> needs sanitizing as well.

ln -sf libbsd-compat.a $RPM_BUILD_ROOT%{_prefix}/%{_lib}/libbsd.a

mkdir -p $RPM_BUILD_ROOT/etc/default

# Take care of setuids
# -- new security review sez that this shouldn't be needed anymore

# This is for ncsd - in glibc 2.2
install -m 644 nscd/nscd.conf $RPM_BUILD_ROOT/etc
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d
install -m 755 nscd/nscd.init $RPM_BUILD_ROOT/etc/rc.d/init.d/nscd

# Workaround hard float ld-linux
#ifarch armv8el armv7hl
#ln -s ld-linux-armhf.so.3 $RPM_BUILD_ROOT/lib/ld-linux.so.3
#endif

# Don't include ld.so.cache
rm -f $RPM_BUILD_ROOT/etc/ld.so.cache

# Include ld.so.conf
echo 'include ld.so.conf.d/*.conf' > $RPM_BUILD_ROOT/etc/ld.so.conf
> $RPM_BUILD_ROOT/etc/ld.so.cache
chmod 644 $RPM_BUILD_ROOT/etc/ld.so.conf
mkdir -p $RPM_BUILD_ROOT/etc/ld.so.conf.d
mkdir -p $RPM_BUILD_ROOT/etc/sysconfig
> $RPM_BUILD_ROOT/etc/sysconfig/nscd

# Include %{_prefix}/%{_lib}/gconv/gconv-modules.cache
> $RPM_BUILD_ROOT%{_prefix}/%{_lib}/gconv/gconv-modules.cache
chmod 644 $RPM_BUILD_ROOT%{_prefix}/%{_lib}/gconv/gconv-modules.cache

strip -g $RPM_BUILD_ROOT%{_prefix}/%{_lib}/*.o

mkdir -p $RPM_BUILD_ROOT%{_prefix}/lib/debug%{_prefix}/%{_lib}
cp -a $RPM_BUILD_ROOT%{_prefix}/%{_lib}/*.a \
  $RPM_BUILD_ROOT%{_prefix}/lib/debug%{_prefix}/%{_lib}/
rm -f $RPM_BUILD_ROOT%{_prefix}/lib/debug%{_prefix}/%{_lib}/*_p.a
# Now strip debugging info from static libraries
pushd $RPM_BUILD_ROOT%{_prefix}/%{_lib}/
for i in *.a; do
  if [ -f $i ]; then
    case "$i" in
    *_p.a) ;;
    *) strip -g -R .comment $i ;;
    esac
  fi
done
popd

# rquota.x and rquota.h are now provided by quota
rm -f $RPM_BUILD_ROOT%{_prefix}/include/rpcsvc/rquota.[hx]

rm -f ${RPM_BUILD_ROOT}/%{_lib}/libnss1-*
rm -f ${RPM_BUILD_ROOT}/%{_lib}/libnss-*.so.1

# Ugly hack for buggy rpm
ln -f ${RPM_BUILD_ROOT}%{_sbindir}/iconvconfig{,.%{_target_cpu}}

rm -f $RPM_BUILD_ROOT/etc/gai.conf

# In F7+ this is provided by rpcbind rpm
rm -f $RPM_BUILD_ROOT%{_sbindir}/rpcinfo

# BUILD THE FILE LIST
{
  find $RPM_BUILD_ROOT \( -type f -o -type l \) \
       \( \
	 -name etc -printf "%%%%config " -o \
	 -name gconv-modules \
	 -printf "%%%%verify(not md5 size mtime) %%%%config(noreplace) " -o \
	 -name gconv-modules.cache \
	 -printf "%%%%verify(not md5 size mtime) " \
	 , \
	 ! -path "*/lib/debug/*" -printf "/%%P\n" \)
  find $RPM_BUILD_ROOT -type d \
       \( -path '*%{_prefix}/share/*' ! -path '*%{_infodir}' -o \
	  -path "*%{_prefix}/include/*" -o \
	  -path "*%{_prefix}/lib/locale/*" \
       \) -printf "%%%%dir /%%P\n"
} | {

  # primary filelist
  SHARE_LANG='s|.*/share/locale/\([^/_]\+\).*/LC_MESSAGES/.*\.mo|%lang(\1) &|'
  LIB_LANG='s|.*/lib/locale/\([^/_]\+\)|%lang(\1) &|'
  # rpm does not handle %lang() tagged files hardlinked together accross
  # languages very well, temporarily disable
  LIB_LANG=''
  sed -e "$LIB_LANG" -e "$SHARE_LANG" \
      -e '\,/etc/\(localtime\|ld\.so\.conf\|ld\.so\.cache\|default\),d' \
      -e '\,/%{_lib}/lib\(pcprofile\|memusage\)\.so,d' \
      -e '\,bin/\(memusage\|mtrace\|xtrace\|pcprofiledump\),d'
} | sort > rpm.filelist

mkdir -p $RPM_BUILD_ROOT%{_prefix}/%{_lib}
mv -f $RPM_BUILD_ROOT/%{_lib}/lib{pcprofile,memusage}.so $RPM_BUILD_ROOT%{_prefix}/%{_lib}

grep '%{_prefix}/include/gnu/stubs-[32164]\+\.h' < rpm.filelist > devel.filelist || :

grep '%{_prefix}/include' < rpm.filelist |
  egrep -v '%{_prefix}/include/(linuxthreads|gnu/stubs-[32164]+\.h)' \
	> headers.filelist

sed -i -e '\|%{_prefix}/%{_lib}/lib.*_p.a|d' \
       -e '\|%{_prefix}/include|d' \
       -e '\|%{_infodir}|d' rpm.filelist

grep '%{_prefix}/%{_lib}/lib.*\.a' < rpm.filelist \
  | grep '/lib\(\(c\|pthread\|nldbl\)_nonshared\|bsd\(\|-compat\)\|g\|ieee\|mcheck\|rpcsvc\)\.a$' \
  >> devel.filelist
grep '%{_prefix}/%{_lib}/lib.*\.a' < rpm.filelist \
  | grep -v '/lib\(\(c\|pthread\|nldbl\)_nonshared\|bsd\(\|-compat\)\|g\|ieee\|mcheck\|rpcsvc\)\.a$' \
  > static.filelist
grep '%{_prefix}/%{_lib}/.*\.o' < rpm.filelist >> devel.filelist
grep '%{_prefix}/%{_lib}/lib.*\.so' < rpm.filelist >> devel.filelist

sed -i -e '\|%{_prefix}/%{_lib}/lib.*\.a|d' \
       -e '\|%{_prefix}/%{_lib}/.*\.o|d' \
       -e '\|%{_prefix}/%{_lib}/lib.*\.so|d' \
       -e '\|%{_prefix}/%{_lib}/linuxthreads|d' \
       -e '\|nscd|d' rpm.filelist

grep '%{_prefix}/bin' < rpm.filelist >> common.filelist
%if 0%{localepackage}
grep '%{_prefix}/lib/locale' < rpm.filelist | grep -v /locale-archive.tmpl >> common.filelist
%endif
mkdir -p $RPM_BUILD_ROOT/%{_libexecdir}/
grep '%{_prefix}/sbin/[^gi]' < rpm.filelist >> common.filelist
grep '%{_prefix}/share' < rpm.filelist | grep -v '%{_prefix}/share/zoneinfo' >> common.filelist

sed -i -e '\|%{_prefix}/bin|d' \
       -e '\|%{_prefix}/lib/locale|d' \
       -e '\|%{_prefix}/sbin/[^gi]|d' \
       -e '\|%{_prefix}/share|d' rpm.filelist > nosegneg.filelist

#echo '%{_prefix}/sbin/tzdata-update' >> common.filelist
echo '%{_prefix}/sbin/nscd' > nscd.filelist

cat > utils.filelist <<EOF
%{_prefix}/%{_lib}/libmemusage.so
%{_prefix}/%{_lib}/libpcprofile.so
#%{_prefix}/bin/memusage
#%{_prefix}/bin/memusagestat
%{_prefix}/bin/mtrace
%{_prefix}/bin/pcprofiledump
%{_prefix}/bin/xtrace
EOF

# /etc/localtime
rm -f $RPM_BUILD_ROOT/etc/localtime
cp -f %{_prefix}/share/zoneinfo/US/Eastern $RPM_BUILD_ROOT/etc/localtime

rm -rf $RPM_BUILD_ROOT%{_prefix}/share/zoneinfo

# Make sure %config files have the same timestamp
touch -r timezone/northamerica $RPM_BUILD_ROOT/etc/localtime
touch -r sunrpc/etc.rpc $RPM_BUILD_ROOT/etc/rpc

#$GCC -Os -static -o tzdata-update %SOURCE12 \
#  -L./build-%{nptl_target_cpu}-linuxnptl
#install -m 700 tzdata-update $RPM_BUILD_ROOT/usr/sbin/tzdata-update

# the last bit: more documentation
rm -rf documentation
mkdir documentation
cp crypt/README.ufc-crypt documentation/README.ufc-crypt
cp timezone/README documentation/README.timezone
cp ChangeLog{,.15,.16} documentation
bzip2 -9 documentation/ChangeLog*
cp posix/gai.conf documentation/

%if 0%{run_glibc_tests}

# Increase timeouts
export TIMEOUTFACTOR=16
parent=$$
echo ====================TESTING=========================
cd build-%{nptl_target_cpu}-linuxnptl
( make %{?_smp_mflags} -k check PARALLELMFLAGS=-s 2>&1
  sleep 10s
  teepid="`ps -eo ppid,pid,command | awk '($1 == '${parent}' && $3 ~ /^tee/) { print $2 }'`"
  [ -n "$teepid" ] && kill $teepid
) | tee check.log || :
cd ..
echo ====================TESTING DETAILS=================
for i in `sed -n 's|^.*\*\*\* \[\([^]]*\.out\)\].*$|\1|p' build-*-linux*/check.log`; do
  echo =====$i=====
  cat $i || :
  echo ============
done
echo ====================TESTING END=====================
PLTCMD='/^Relocation section .*\(\.rela\?\.plt\|\.rela\.IA_64\.pltoff\)/,/^$/p'
echo ====================PLT RELOCS LD.SO================
readelf -Wr $RPM_BUILD_ROOT/%{_lib}/ld-*.so | sed -n -e "$PLTCMD"
echo ====================PLT RELOCS LIBC.SO==============
readelf -Wr $RPM_BUILD_ROOT/%{_lib}/libc-*.so | sed -n -e "$PLTCMD"
echo ====================PLT RELOCS END==================

%endif

pushd $RPM_BUILD_ROOT/usr/%{_lib}/
$GCC -r -nostdlib -o libpthread.o -Wl,--whole-archive ./libpthread.a
rm libpthread.a
ar rcs libpthread.a libpthread.o
rm libpthread.o
# Delete the static libraries for profile.
ls *_p.a |xargs rm -f
# Delete the static libraries for debug.
ls ./debug/usr/lib/*.a |xargs rm -f
popd


rm -f $RPM_BUILD_ROOT%{_infodir}/dir

%ifarch %{auxarches}

echo Cutting down the list of unpackaged files
>> debuginfocommon.filelist
sed -e '/%%dir/d;/%%config/d;/%%verify/d;s/%%lang([^)]*) //;s#^/*##' \
    common.filelist devel.filelist static.filelist headers.filelist \
    utils.filelist nscd.filelist debuginfocommon.filelist |
(cd $RPM_BUILD_ROOT; xargs --no-run-if-empty rm -f 2> /dev/null || :)

%else

mkdir -p $RPM_BUILD_ROOT/var/{db,run}/nscd
touch $RPM_BUILD_ROOT/var/{db,run}/nscd/{passwd,group,hosts,services}
touch $RPM_BUILD_ROOT/var/run/nscd/{socket,nscd.pid}
%endif

%ifnarch %{auxarches}
%if 0%{localepackage}
> $RPM_BUILD_ROOT/%{_prefix}/lib/locale/locale-archive
%endif
%endif

mkdir -p $RPM_BUILD_ROOT/var/cache/ldconfig
> $RPM_BUILD_ROOT/var/cache/ldconfig/aux-cache

mkdir -p $RPM_BUILD_ROOT/etc
cp -f %{SOURCE5} $RPM_BUILD_ROOT/etc/nsswitch.conf

%postun -p /sbin/ldconfig

#%posttrans common
#/bin/ls /usr/lib/locale/ | /bin/grep _ | /usr/bin/xargs -I {} /bin/rm -rf /usr/lib/locale/{}
#/bin/rm -rf /usr/lib/locale/C.UTF-8
#/bin/find /usr/share/locale/ -name libc.mo | /bin/grep -v en_GB | /usr/bin/xargs -I {} /bin/rm {}

#%triggerin common -p /usr/sbin/tzdata-update -- tzdata


%pre headers
# this used to be a link and it is causing nightmares now
if [ -L %{_prefix}/include/scsi ] ; then
  rm -f %{_prefix}/include/scsi
fi


%post utils -p /sbin/ldconfig

%postun utils -p /sbin/ldconfig

%pre -n nscd
/usr/sbin/useradd -M -o -r -d / -s /sbin/nologin \
  -c "NSCD Daemon" -u 28 nscd > /dev/null 2>&1 || :

%post -n nscd
/sbin/chkconfig --add nscd

%preun -n nscd
if [ $1 = 0 ] ; then
  service nscd stop > /dev/null 2>&1
  /sbin/chkconfig --del nscd
fi

%postun -n nscd
if [ $1 = 0 ] ; then
  /usr/sbin/userdel nscd > /dev/null 2>&1 || :
fi
if [ "$1" -ge "1" ]; then
  service nscd condrestart > /dev/null 2>&1 || :
fi


%clean
rm -rf "$RPM_BUILD_ROOT"
rm -f *.filelist*

%files -f rpm.filelist
%defattr(-,root,root)
%verify(not md5 size mtime) %config(noreplace) /etc/localtime
%verify(not md5 size mtime) %config(noreplace) /etc/ld.so.conf
%dir /etc/ld.so.conf.d
%dir %{_libexecdir}/getconf
%dir %{_prefix}/%{_lib}/gconv
%dir %attr(0700,root,root) /var/cache/ldconfig
%attr(0600,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/cache/ldconfig/aux-cache
%attr(0644,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /etc/ld.so.cache
%doc README NEWS INSTALL BUGS PROJECTS CONFORMANCE
%doc COPYING COPYING.LIB LICENSES
%doc hesiod/README.hesiod
%attr(0644,root,-) /etc/nsswitch.conf
/usr/share/license/%{name}
%manifest %{name}.manifest

%ifnarch %{auxarches}
%files -f common.filelist common
%defattr(-,root,root)

%if 0%{localepackage}
%dir %{_prefix}/lib/locale
%attr(0644,root,root) %verify(not md5 size mtime mode) %ghost %config(missingok,noreplace) %{_prefix}/lib/locale/locale-archive
%endif

%dir %attr(755,root,root) /etc/default
%doc documentation/*
/usr/share/license/%{name}
%manifest %{name}.manifest

%files -f devel.filelist devel
%defattr(-,root,root)

%files -f static.filelist static
%defattr(-,root,root)

%files -f headers.filelist headers
%defattr(-,root,root)

%files -f utils.filelist utils
%defattr(-,root,root)

%files -f nscd.filelist -n nscd
%defattr(-,root,root)
%config(noreplace) /etc/nscd.conf
%config /etc/rc.d/init.d/nscd
%dir %attr(0755,root,root) /var/run/nscd
%dir %attr(0755,root,root) /var/db/nscd
%attr(0644,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/run/nscd/nscd.pid
%attr(0666,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/run/nscd/socket
%attr(0600,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/run/nscd/passwd
%attr(0600,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/run/nscd/group
%attr(0600,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/run/nscd/hosts
%attr(0600,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/run/nscd/services
%attr(0600,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/db/nscd/passwd
%attr(0600,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/db/nscd/group
%attr(0600,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/db/nscd/hosts
%attr(0600,root,root) %verify(not md5 size mtime) %ghost %config(missingok,noreplace) /var/db/nscd/services
%ghost %config(missingok,noreplace) /etc/sysconfig/nscd
%endif
