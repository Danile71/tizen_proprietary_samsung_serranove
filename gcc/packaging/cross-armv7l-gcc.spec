# Combined gcc / cross-armv*-gcc(-accel) specfile
%ifarch x86_64
%define x64 x64
%endif
#define gcc_package_version 48
Name: cross-armv7l-gcc

# crossbuild / accelerator section
# \/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
%define crossbuild 0
%define accelerator_crossbuild 0
%if "%{name}" != "gcc%{?gcc_package_version}"
# this is the ix86 -> arm cross compiler (cross-armv*-gcc%{?gcc_package_version})
#
# cross arch retrieval
%define crossarch %{lua: x=string.gsub(rpm.expand("%{name}"), "cross%-(.*)%-"..rpm.expand("gcc%{?gcc_package_version}"), "%1"); print(x)}
# We set requires/provides by hand and disable post-build-checks.
# Captain Trunk: Sledge, you cannot disarm that nuclear bomb!
# Sledge Hammer: Trust me, I know what I'm doing.
AutoReqProv: 0
AutoReq: false
#!BuildIgnore: rpmlint-Moblin rpmlint-mini post-build-checks
# cross platform
%define cross_gcc_target_platform %{crossarch}-tizen-linux-gnueabi
# gcc_target_platform holds the host (executing the compiler)
# cross_gcc_target_platform holds the target (for which the compiler is producing binaries)
# prefix for cross compiler
%define _prefix /opt/cross
# strip of 'foreign arch' symbols fails
%define __strip /bin/true
# sysroot for cross-compiler
%define crosssysroot %{_prefix}/%{cross_gcc_target_platform}/sys-root
# flag
%define crossbuild 1
# macros in buildrequires is hard to expand for the scheduler (e.g. crossarch) which would make this easier.
#BuildRequires: cross-%{crossarch}-glibc cross-%{crossarch}-glibc-devel cross-%{crossarch}-glibc-headers
#BuildRequires: cross-%{crossarch}-kernel-headers cross-%{crossarch}-binutils
# Fixme: find way to make this without listing every package
%if "%{name}" == "cross-armv7l-gcc%{?gcc_package_version}"
BuildRequires: cross-armv7l-glibc cross-armv7l-glibc-devel cross-armv7l-glibc-headers
BuildRequires: cross-armv7l-kernel-headers cross-armv7l-binutils
%define crossextraconfig --with-float=softfp --with-fpu=vfpv3 --with-arch=armv7-a
%endif
# Fixme: see above
%if "%{name}" == "cross-armv7l-gcc%{?gcc_package_version}-accel-x86" || "%{name}" == "cross-armv7l-gcc%{?gcc_package_version}-accel-x64"
BuildRequires: cross-armv7l-glibc cross-armv7l-glibc-devel cross-armv7l-glibc-headers
BuildRequires: cross-armv7l-kernel-headers cross-armv7l-binutils
%define crossextraconfig --with-float=softfp --with-fpu=vfpv3 --with-arch=armv7-a
%endif
# single target atm.
ExclusiveArch: %ix86 x86_64
#
# special handling for Tizen ARM build acceleration
# cross-armv*-gcc%{?gcc_package_version}-accel
%if "%{lua: x=string.gsub(rpm.expand("%{name}"), "cross%-.*%-"..rpm.expand("gcc%{?gcc_package_version}").."%-(accel)%-?.*", "%1"); print(x)}" == "accel"
# cross architecture
%define crossarch %{lua: x=string.gsub(rpm.expand("%{name}"), "cross%-(.*)%-"..rpm.expand("gcc%{?gcc_package_version}").."%-accel%-?.*", "%1"); print(x)}
# cross target platform
%define cross_gcc_target_platform %{crossarch}-tizen-linux-gnueabi
# prefix - as it's going to "replace" the original compiler ...
%define _prefix /usr
# cross-build sets sysroot as default , we need to override this a bit
%define crosssysroot /
# where to find the libs during the build
%define crossbuildsysroot /opt/cross/%{cross_gcc_target_platform}/sys-root
# flags
%define crossbuild 1
%define accelerator_crossbuild 1
# where to find the libs at runtime
%define newrpath /emul/ia32-linux/%{_lib}:/emul/ia32-linux/usr/%{_lib}
%define _build_name_fmt    %%{ARCH}/%%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.dontuse.rpm
%endif
# end special accel
%endif
# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
# end crossbuild / accelerator section

%global gcc_version 4.9.2
%global gcc_release 2015.02
%global gcc_dir_version %{lua: x=string.gsub(rpm.expand("%gcc_version"), "(%d*%.%d*).*", "%1"); print(x)}
%if 0%{?gcc_package_version}
%global binsuffix -%gcc_dir_version
%else$
%global gcc_old_package_version %{lua: x=string.gsub(rpm.expand("%gcc_dir_version"), "%.", ""); print(x)}
%endif
%global release_prefix %{gcc_release}
#global _unpackaged_files_terminate_build 0
%ifarch %{arm}
%global build_cloog 0
%else
%if %{crossbuild}
%global build_cloog 1
%else
%global build_cloog 1
%endif
%endif
#global multilib_64_archs x86_64
%ifarch x86_64
%global multilib_32_arch i686
%endif

Summary: Various compilers (C, C++, Objective-C, Java, ...)
Version: %{gcc_version}
Release: %{gcc_release}
License: GPLv3+, GPLv3+ with exceptions and GPLv2+ with exceptions
Group: Development/Languages
URL: http://gcc.gnu.org
Source0: gcc-linaro-%{gcc_dir_version}-%{gcc_release}.tar.xz
#Source1: libgcc_post_upgrade.c
Source10: gmp-6.0.0a.tar.bz2
Source11: mpfr-3.1.2.tar.gz
Source12: mpc-1.0.tar.gz
%if %{build_cloog}
Source13: isl-0.12.2.tar.bz2
Source14: cloog-0.18.1.tar.gz
%endif
Source100: gcc-rpmlintrc
Source200: baselibs.conf
Source300: precheckin.sh
Source301: aaa_README.PACKAGER
Source1001: gcc.manifest
# http://gcc.gnu.org/bugzilla/show_bug.cgi?id=53113

BuildRequires: binutils >= 2.19.51.0.14
BuildRequires: glibc-devel >= 2.4.90-13
BuildRequires: glibc-static
BuildRequires: zlib-devel
BuildRequires: bison
BuildRequires: flex
BuildRequires: elfutils-devel >= 0.72

Requires: cpp%{?gcc_package_version} = %{version}-%{release}
Requires: libgcc >= %{version}-%{release}
Requires: libgomp >= %{version}-%{release}
Requires: libitm >= %{version}-%{release}
Requires: libasan >= %{version}-%{release}
%ifarch x86_64
Requires: libtsan >= %{version}-%{release}
Requires: liblsan >= %{version}-%{release}
%endif
Requires: libatomic >= %{version}-%{release}
Requires: libubsan >= %{version}-%{release}
%ifnarch %arm
Requires: libvtv >= %{version}-%{release}
%endif
Requires: glibc-devel
Requires: binutils >= 2.19.51.0.14

%if !%{crossbuild}
Obsoletes: gcc%{?gcc_package_version} < %{version}-%{release}
%if 0%{?gcc_old_package_version}
Provides: gcc%{?gcc_old_package_version}
Obsoletes: gcc%{?gcc_old_package_version}
%endif
AutoReq: true
# /!crossbuild
%endif

#We need -gnueabi indicator for ARM
%ifnarch %{arm}
%global _gnu %{nil}
%endif
%global gcc_target_platform %{_target_platform}

%description
The gcc package contains the GNU Compiler Collection version %{version}.
You'll need this package in order to compile C code.

%package -n cpp%{?gcc_package_version}
Summary: The C Preprocessor
Group: Development/Languages
Obsoletes: cpp%{?gcc_package_version} < %{version}-%{release}
%if 0%{?gcc_old_package_version}
Obsoletes: cpp%{?gcc_old_package_version}
%endif
AutoReq: true

%description -n cpp%{?gcc_package_version}
Cpp is the GNU C-Compatible Compiler Preprocessor.
Cpp is a macro processor which is used automatically
by the C compiler to transform your program before actual
compilation. It is called a macro processor because it allows
you to define macros, abbreviations for longer
constructs.

The C preprocessor provides four separate functionalities: the
inclusion of header files (files of declarations that can be
substituted into your program); macro expansion (you can define macros,
and the C preprocessor will replace the macros with their definitions
throughout the program); conditional compilation (using special
preprocessing directives, you can include or exclude parts of the
program according to various conditions); and line control (if you use
a program to combine or rearrange source files into an intermediate
file which is then compiled, you can use line control to inform the
compiler about where each source line originated).

You should install this package if you are a C programmer and you use
macros.

%package -n libgcc%{?gcc_package_version}
Summary: GCC version %{version} shared support library
Group: System Environment/Libraries
Provides: libgcc = %{version}-%{release}
Obsoletes: libgcc%{?gcc_package_version} < %{version}-%{release}
%if 0%{?gcc_old_package_version}
Obsoletes: libgcc%{?gcc_old_package_version}
%endif
AutoReq: false

%description -n libgcc%{?gcc_package_version}
This package contains GCC shared support library which is needed
e.g. for exception handling support.

%package c++
Summary: C++ support for GCC
Group: Development/Languages
Requires: gcc%{?gcc_package_version} = %{version}-%{release}
Requires: libstdc++%{?gcc_package_version}-devel = %{version}-%{release}
Obsoletes: gcc%{?gcc_package_version}-c++ < %{version}-%{release}
%if 0%{?gcc_old_package_version}
Provides: gcc%{?gcc_old_package_version}-c++
Obsoletes: gcc%{?gcc_old_package_version}-c++
%endif
AutoReq: true

%description c++
This package adds C++ support to the GNU Compiler Collection.
It includes support for most of the current C++ specification,
including templates and exception handling.

%package -n libstdc++%{?gcc_package_version}
Summary: GNU Standard C++ Library
Group: System Environment/Libraries
Provides: libstdc++ = %{version}-%{release}
Obsoletes: libstdc++%{?gcc_package_version} < %{version}-%{release}
%if 0%{?gcc_old_package_version}
Obsoletes: libstdc++%{?gcc_old_package_version}
%endif
AutoReq: true
Requires: glibc

%description -n libstdc++%{?gcc_package_version}
The libstdc++ package contains a rewritten standard compliant GCC Standard
C++ Library.

%package -n libstdc++%{?gcc_package_version}-devel
Summary: Header files and libraries for C++ development
Group: Development/Libraries
Requires: glibc-devel
Requires: libstdc++ >= %{version}-%{release}
Obsoletes: libstdc++%{?gcc_package_version}-devel < %{version}-%{release}
%if 0%{?gcc_old_package_version}
Obsoletes: libstdc++%{?gcc_old_package_version}-devel
%endif
AutoReq: true

%description -n libstdc++%{?gcc_package_version}-devel
This is the GNU implementation of the standard C++ libraries.  This
package includes the header files and libraries needed for C++
development. This includes rewritten implementation of STL.

%package -n libstdc++%{?gcc_package_version}-docs
Summary: Documentation for the GNU standard C++ library
Group: Development/Libraries
Obsoletes: libstdc++%{?gcc_package_version}-docs < %{version}-%{release}
%if 0%{?gcc_old_package_version}
Obsoletes: libstdc++%{?gcc_old_package_version}-docs
%endif
AutoReq: true

%description -n libstdc++%{?gcc_package_version}-docs
Manual, doxygen generated API information and Frequently Asked Questions
for the GNU standard C++ library.

%package -n libgomp%{?gcc_package_version}
Summary: GCC OpenMP v3.0 shared support library
Group: System Environment/Libraries
Provides: libgomp = %{version}-%{release}
Obsoletes: libgomp%{?gcc_old_package_version} < %{version}-%{release}
%if 0%{?gcc_old_package_version}
Obsoletes: libgomp%{?gcc_old_package_version}
%endif

%description -n libgomp%{?gcc_package_version}
This package contains GCC shared support library which is needed
for OpenMP v3.0 support.

%package -n libitm%{?gcc_package_version}
Summary:        The GNU Compiler Transactional Memory Runtime Library
License:        MIT
Group:          Development/Languages/C and C++
Provides: libitm = %{version}-%{release}
Obsoletes: libitm%{?gcc_old_package_version} < %{version}-%{release}
%if 0%{?gcc_old_package_version}
Obsoletes: libitm%{?gcc_old_package_version}
%endif

%description -n libitm%{?gcc_package_version}
The runtime library needed to run programs compiled with the
-fgnu-tm option of the GNU Compiler Collection (GCC).

%package -n libasan%{?gcc_package_version}
Summary:        The GNU Compiler Address Sanitizer Runtime Library
License:        MIT
Group:          Development/Languages/C and C++
Provides: libasan = %{version}-%{release}
Obsoletes: libasan%{?gcc_package_version} < %{version}-%{release}
%if 0%{?gcc_old_package_version}
Obsoletes: libasan%{?gcc_old_package_version}
%endif

%description -n libasan%{?gcc_package_version}
The runtime library needed to run programs compiled with the
-fsanitize=address option of the GNU Compiler Collection (GCC).

%ifarch x86_64
%package -n libtsan%{?gcc_package_version}
Summary:        The GNU Compiler Thread Sanitizer Runtime Library
License:        MIT
Group:          Development/Languages/C and C++
Provides: libtsan = %{version}-%{release}
Obsoletes: libtsan%{?gcc_package_version} < %{version}-%{release}
%if 0%{?gcc_old_package_version}
Obsoletes: libtsan%{?gcc_old_package_version}
%endif

%description -n libtsan%{?gcc_package_version}
The runtime library needed to run programs compiled with the
-fsanitize=thread option of the GNU Compiler Collection (GCC).

%package -n liblsan%{?gcc_package_version}
Summary:        The GNU Compiler Leak Sanitizer Runtime Library
License:        MIT
Group:          Development/Languages/C and C++
Provides:       liblsan = %{version}-%{release}
Obsoletes: liblsan%{?gcc_package_version} < %{version}-%{release}
%if 0%{?gcc_old_package_version}
Obsoletes: liblsan%{?gcc_old_package_version}
%endif

%description -n liblsan%{?gcc_package_version}
The runtime library needed to run programs compiled with the
-fsanitize=leak option of the GNU Compiler Collection (GCC).
%endif

%package -n libatomic%{?gcc_package_version}
Summary:        The GNU Compiler Atomic Operations Runtime Library
License:        GPL-3.0-with-GCC-exception
Group:          Development/Languages/C and C++
Provides:       libatomic = %{version}-%{release}
Obsoletes: libatomic%{?gcc_package_version} < %{version}-%{release}
%if 0%{?gcc_old_package_version}
Obsoletes: libatomic%{?gcc_old_package_version}
%endif

%description -n libatomic%{?gcc_package_version}
The runtime library for atomic operations of the GNU Compiler Collection (GCC).

%package -n libubsan%{?gcc_package_version}
Summary:        The GNU Compiler Undefined Sanitizer Runtime Library
License:        MIT
Group:          Development/Languages/C and C++
Provides:       libubsan = %{version}-%{release}
Obsoletes: libubsan%{?gcc_package_version} < %{version}-%{release}
%if 0%{?gcc_old_package_version}
Obsoletes: libubsan%{?gcc_old_package_version}
%endif

%description -n libubsan%{?gcc_package_version}
The runtime library needed to run programs compiled with the
-fsanitize=undefined option of the GNU Compiler Collection (GCC).

%ifnarch %arm
%package -n libvtv%{?gcc_package_version}
Summary:        The GNU Compiler Vtable Verifier Runtime Library
License:        MIT
Group:          Development/Languages/C and C++
Provides:       libvtv = %{version}-%{release}
Obsoletes: libvtv%{?gcc_package_version} < %{version}-%{release}
%if 0%{?gcc_old_package_version}
Obsoletes: libvtv%{?gcc_old_package_version}
%endif

%description -n libvtv%{?gcc_package_version}
The runtime library needed to run programs compiled with the
-fvtable-verify option of the GNU Compiler Collection (GCC).
%endif

%package -n gcc%{?gcc_package_version}-multilib
Summary: for 64bit multilib support
Group: System Environment/Libraries
AutoReq: true

%description -n gcc%{?gcc_package_version}-multilib
This is one set of libraries which support 64bit multilib on top of
32bit enviroment from compiler side.

%prep
%setup -q -n gcc-linaro-%{gcc_dir_version}-%{gcc_release}

tar xf %{SOURCE10}
ln -sf gmp-6.0.0 gmp
tar xf %{SOURCE11}
ln -sf mpfr-3.1.2 mpfr
tar xf %{SOURCE12}
ln -sf mpc-1.0 mpc
%if %{build_cloog}
tar xf %{SOURCE13}
ln -sf isl-0.12.2 isl
tar xf %{SOURCE14}
ln -sf cloog-0.18.1 cloog
%endif

cp %{SOURCE1001} .

echo %{gcc_version} > gcc/FULL-VER
cat gcc/FULL-VER | cut -d '.' -f 1-2 > gcc/BASE-VER
echo "" > gcc/DEV-PHASE

cp -a libstdc++-v3/config/cpu/i{4,3}86/atomicity.h

./contrib/gcc_update --touch

LC_ALL=C sed -i -e 's/\xa0/ /' gcc/doc/options.texi

%build
rm -fr obj-%{gcc_target_platform}
mkdir obj-%{gcc_target_platform}
cd obj-%{gcc_target_platform}


CC=gcc
OPT_FLAGS=`echo %{optflags}|sed -e 's/\(-Wp,\)\?-D_FORTIFY_SOURCE=[12]//g'`
OPT_FLAGS=`echo $OPT_FLAGS|sed -e 's/-m64//g;s/-m32//g;s/-m31//g'`
%ifarch %{ix86}
OPT_FLAGS=`echo $OPT_FLAGS|sed -e 's/-march=i.86//g'`
%endif
OPT_FLAGS=`echo "$OPT_FLAGS" | sed -e 's/[[:blank:]]\+/ /g'`
case "$OPT_FLAGS" in
  *-fasynchronous-unwind-tables*)
    sed -i -e 's/-fno-exceptions /-fno-exceptions -fno-asynchronous-unwind-tables/' \
      ../gcc/Makefile.in
    ;;
esac

%ifarch %arm
# gcc 45 segfaults on O2g.gch generation. (cc1plus)
%define ARM_EXTRA_CONFIGURE --enable-libstdcxx-pch
# for armv7l reset the gcc specs
%ifarch armv7l armv7el
%define ARM_EXTRA_CONFIGURE --enable-libstdcxx-pch --with-fpu=vfpv3-d16 --with-arch=armv7-a
%endif
%endif

%if %{crossbuild}
# cross build
export PATH=/opt/cross/bin:$PATH
# strip all after -march . no arch specific options in cross-compiler build .
# -march=core2 -mssse3 -mtune=atom -mfpmath=sse -fasynchronous-unwi
export OPT_FLAGS=`echo "$OPT_FLAGS" | sed -e "s#\-march=.*##g"`
export OPT_FLAGS=`echo "$OPT_FLAGS" | sed -e "s#\-mtune=.*##g"`
%if %{accelerator_crossbuild}
# adding -rpath to the special crosscompiler
export OPT_FLAGS="$OPT_FLAGS -Wl,-rpath,/emul/ia32-linux/usr/%{_lib}:/emul/ia32-linux/%{_lib}:/usr/%{_lib}:/%{_lib}:/usr/lib:/lib"
%endif
%endif

export OPT_FLAGS="`echo $OPT_FLAGS | sed -e 's/-Wall / /g' -e 's/-fexceptions / /g'`"
CC="$CC" CFLAGS="$OPT_FLAGS -fno-use-linker-plugin" CXXFLAGS="$OPT_FLAGS" XCFLAGS="$OPT_FLAGS" TCFLAGS="$OPT_FLAGS" \
	../configure --prefix=%{_prefix} --mandir=%{_mandir} --infodir=%{_infodir} --libexecdir=%{_libexecdir} \
	--enable-shared --enable-linker-build-id \
%if 0%{?gcc_package_version}
	--program-suffix=%{binsuffix} \
%endif
%ifarch %{arm}
	--enable-checking=release \
	--disable-sjlj-exceptions \
	%ARM_EXTRA_CONFIGURE \
%else
%if %{crossbuild}
	--build=%{gcc_target_platform} \
	--host=%{gcc_target_platform} \
	--target=%{cross_gcc_target_platform} \
	--disable-checking \
%else
	--enable-checking=release \
	--enable-multiarch \
%endif
%endif
	--disable-multilib \
	--with-system-zlib --enable-__cxa_atexit --disable-libunwind-exceptions \
	--enable-gnu-unique-object --enable-lto \
	--without-included-gettext \
	--enable-threads=posix --enable-libstdcxx-threads \
	--enable-nls \
	--disable-bootstrap \
	--enable-clocale=gnu --enable-libstdcxx-time=yes \
	--enable-plugin --enable-gold --enable-ld=default --with-plugin-ld=ld.gold \
	--enable-languages=c,c++ \
	--disable-libgcj \
	--disable-libquadmath \
	--disable-libcilkrts \
        --with-bugurl="http://bugs.tizen.org/" \
        --with-pkgversion="Tizen/Linaro GCC %{gcc_version} %{release}" \
%if !%{crossbuild}
%ifarch %{ix86} x86_64
	--with-tune=generic \
%endif
%ifarch %{ix86}
	--with-arch=i686 \
%endif
%ifarch x86_64
	--with-arch_32=i686 \
%endif
	--build=%{gcc_target_platform}  || ( cat config.log ; exit 1 )
#end for x86
%else
%if !%{accelerator_crossbuild}
	%{crossextraconfig} \
	--with-sysroot=%{crosssysroot}
#end for cross-compiler
%else
	%{crossextraconfig} \
	--program-transform-name='s/%{cross_gcc_target_platform}-//' \
	--with-gxx-include-dir=%{_prefix}/include/c++/%{gcc_dir_version} \
	--with-build-sysroot=%{crossbuildsysroot} \
	--with-sysroot=%{crosssysroot}
#end for special cross-compiler
%endif
%endif

make %{?_smp_mflags} BOOT_CFLAGS="$OPT_FLAGS"


# Make generated man pages even if Pod::Man is not new enough
perl -pi -e 's/head3/head2/' ../contrib/texi2pod.pl
for i in ../gcc/doc/*.texi; do
  cp -a $i $i.orig; sed 's/ftable/table/' $i.orig > $i
done
make -C gcc generated-manpages
for i in ../gcc/doc/*.texi; do mv -f $i.orig $i; done
cd ..


%install
rm -fr %{buildroot}

cd obj-%{gcc_target_platform}

%if !%{crossbuild}
# native
TARGET_PLATFORM=%{gcc_target_platform}
# There are some MP bugs in libstdc++ Makefiles
make -C %{gcc_target_platform}/libstdc++-v3
%else
# cross build
export PATH=/opt/cross/bin:$PATH
# strip all after -march . no arch specific options in cross-compiler build .
# -march=core2 -mssse3 -mtune=atom -mfpmath=sse -fasynchronous-unwi
export OPT_FLAGS=`echo "$OPT_FLAGS" | sed -e "s#\-march=.*##g"`
#echo "$OPT_FLAGS"
#TARGET_PLATFORM=%{cross_gcc_target_platform}
# There are some MP bugs in libstdc++ Makefiles
#make -C %{cross_gcc_target_platform}/libstdc++-v3
%endif

make DESTDIR=%{buildroot} install
make DESTDIR=%{buildroot} install-gcc
make DESTDIR=%{buildroot} install-lto-plugin
#prefix=%{buildroot}%{_prefix} mandir=%{buildroot}%{_mandir} \
#  infodir=%{buildroot}%{_infodir} install

%if !%{crossbuild}
# native
# \/\/\/

FULLPATH=%{buildroot}%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}
FULLEPATH=%{buildroot}%{_libexecdir}/gcc/%{gcc_target_platform}/%{gcc_dir_version}

ln -sf gcc%{?binsuffix} %{buildroot}%{_prefix}/bin/cc%{?binsuffix}
mkdir -p %{buildroot}/lib
ln -sf ..%{_prefix}/bin/cpp%{?binsuffix} %{buildroot}/lib/cpp%{?binsuffix}
rm -rf %{buildroot}%{_infodir}
rm -rf %{buildroot}%{_mandir}/man7
rm -rf %{buildroot}%{_datadir}/locale

cxxconfig="`find %{gcc_target_platform}/libstdc++-v3/include -name c++config.h`"
for i in `find %{gcc_target_platform}/[36]*/libstdc++-v3/include -name c++config.h 2>/dev/null`; do
  if ! diff -up $cxxconfig $i; then
    cat > %{buildroot}%{_prefix}/include/c++/%{gcc_dir_version}/%{gcc_target_platform}/bits/c++config.h <<EOF
#ifndef _CPP_CPPCONFIG_WRAPPER
#define _CPP_CPPCONFIG_WRAPPER 1
#include <bits/wordsize.h>
#if __WORDSIZE == 32
%ifarch %{multilib_64_archs}
`cat $(find %{gcc_target_platform}/32/libstdc++-v3/include -name c++config.h)`
%else
`cat $(find %{gcc_target_platform}/libstdc++-v3/include -name c++config.h)`
%endif
#else
%ifarch %{multilib_64_archs}
`cat $(find %{gcc_target_platform}/libstdc++-v3/include -name c++config.h)`
%else
`cat $(find %{gcc_target_platform}/64/libstdc++-v3/include -name c++config.h)`
%endif
#endif
#endif
EOF
    break
  fi
done

for f in `find %{buildroot}%{_prefix}/include/c++/%{gcc_dir_version}/%{gcc_target_platform}/ -name c++config.h`; do
  for i in 1 2 4 8; do
    sed -i -e 's/#define _GLIBCXX_ATOMIC_BUILTINS_'$i' 1/#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_'$i'\
&\
#endif/' $f
  done
done

# Nuke bits/stdc++.h.gch dirs
# 1) there is no bits/stdc++.h header installed, so when gch file can't be
#    used, compilation fails
# 2) sometimes it is hard to match the exact options used for building
#    libstdc++-v3 or they aren't desirable
# 3) there are multilib issues, conflicts etc. with this
# 4) it is huge
# People can always precompile on their own whatever they want, but
# shipping this for everybody is unnecessary.
rm -rf %{buildroot}%{_prefix}/include/c++/%{gcc_dir_version}/%{gcc_target_platform}/bits/stdc++.h.gch


if [ -n "$FULLLPATH" ]; then
  mkdir -p $FULLLPATH
else
  FULLLPATH=$FULLPATH
fi

find %{buildroot} -name \*.la | xargs rm -f

mkdir -p %{buildroot}/%{_lib}
mv -f %{buildroot}%{_prefix}/%{_lib}/libgcc_s.so.1 %{buildroot}/%{_lib}/libgcc_s-%{gcc_dir_version}.so.1
chmod 755 %{buildroot}/%{_lib}/libgcc_s-%{gcc_dir_version}.so.1
ln -sf libgcc_s-%{gcc_dir_version}.so.1 %{buildroot}/%{_lib}/libgcc_s.so.1
ln -sf /%{_lib}/libgcc_s.so.1 %{buildroot}/%{_libdir}/libgcc_s.so
ln -sf /%{_lib}/libgcc_s.so.1 $FULLPATH/libgcc_s.so
%ifarch %{multilib_64_archs}
ln -sf /lib/libgcc_s.so.1 $FULLPATH/32/libgcc_s.so
%endif

mkdir -p %{buildroot}%{_libexecdir}/getconf
if gcc/xgcc -B gcc/ -E -dD -xc /dev/null | grep __LONG_MAX__.*2147483647; then
  ln -sf POSIX_V6_ILP32_OFF32 %{buildroot}%{_libexecdir}/getconf/default
else
  ln -sf POSIX_V6_LP64_OFF64 %{buildroot}%{_libexecdir}/getconf/default
fi


mkdir -p %{buildroot}%{_datadir}/gdb/auto-load/%{_prefix}/%{_lib}
mv -f %{buildroot}%{_prefix}/%{_lib}/libstdc++*gdb.py* \
      %{buildroot}%{_datadir}/gdb/auto-load/%{_prefix}/%{_lib}/

pushd $FULLPATH
if [ "%{_lib}" = "lib" ]; then
ln -sf ../../../libstdc++.so.6.* libstdc++.so
ln -sf ../../../libgomp.so.1.* libgomp.so
ln -sf ../../../libatomic.so.1.* libatomic.so
ln -sf ../../../libitm.so.1.* libitm.so
ln -sf ../../../libasan.so.1.* libasan.so
%ifarch x86_64
ln -sf ../../../libtsan.so.0.* libtsan.so
ln -sf ../../../liblsan.so.0.* liblsan.so
%endif
ln -sf ../../../libubsan.so.0.* libubsan.so
%ifnarch %arm
ln -sf ../../../libvtv.so.0.* libvtv.so
%endif
else
ln -sf ../../../../%{_lib}/libstdc++.so.6.* libstdc++.so
ln -sf ../../../../%{_lib}/libgomp.so.1.* libgomp.so
ln -sf ../../../../%{_lib}/libatomic.so.1.* libatomic.so
ln -sf ../../../../%{_lib}/libitm.so.1.* libitm.so
ln -sf ../../../../%{_lib}/libasan.so.1.* libasan.so
%ifarch x86_64
ln -sf ../../../../%{_lib}/libtsan.so.0.* libtsan.so
ln -sf ../../../../%{_lib}/liblsan.so.0.* liblsan.so
%endif
ln -sf ../../../../%{_lib}/libubsan.so.0.* libubsan.so
%ifnarch %arm
ln -sf ../../../../%{_lib}/libvtv.so.0.* libvtv.so
%endif
fi
mv -f %{buildroot}%{_prefix}/%{_lib}/libstdc++.*a $FULLLPATH/
mv -f %{buildroot}%{_prefix}/%{_lib}/libsupc++.*a .
mv -f %{buildroot}%{_prefix}/%{_lib}/libgomp.*a .
mv -f %{buildroot}%{_prefix}/%{_lib}/libgomp.spec .
mv -f %{buildroot}%{_prefix}/%{_lib}/libatomic.a .
mv -f %{buildroot}%{_prefix}/%{_lib}/libitm.a .
mv -f %{buildroot}%{_prefix}/%{_lib}/libitm.spec .
mv -f %{buildroot}%{_prefix}/%{_lib}/libsanitizer.spec .
mv -f %{buildroot}%{_prefix}/%{_lib}/libasan.a .
mv -f %{buildroot}%{_prefix}/%{_lib}/libasan_preinit.o .
%ifarch x86_64
mv -f %{buildroot}%{_prefix}/%{_lib}/libtsan.a .
mv -f %{buildroot}%{_prefix}/%{_lib}/liblsan.a .
%endif
mv -f %{buildroot}%{_prefix}/%{_lib}/libubsan.a .
%ifnarch %arm
mv -f %{buildroot}%{_prefix}/%{_lib}/libvtv.a .
%endif

rm -f %{buildroot}%{_prefix}/%{_lib}/libgomp.so
rm -f %{buildroot}%{_prefix}/%{_lib}/libatomic.so
rm -f %{buildroot}%{_prefix}/%{_lib}/libitm.so
rm -f %{buildroot}%{_prefix}/%{_lib}/libasan.so
%ifarch x86_64
rm -f %{buildroot}%{_prefix}/%{_lib}/libtsan.so
rm -f %{buildroot}%{_prefix}/%{_lib}/liblsan.so
%endif
rm -f %{buildroot}%{_prefix}/%{_lib}/libubsan.so
%ifnarch %arm
rm -f %{buildroot}%{_prefix}/%{_lib}/libvtv.so
%endif

%ifarch %{multilib_64_archs}
mkdir -p 32
ln -sf ../`echo ../../../../lib64/libstdc++.so.6.* | sed s~/../lib64/~/~` 32/libstdc++.so
ln -sf ../`echo ../../../../lib64/libgomp.so.1.* | sed s~/../lib64/~/~` 32/libgomp.so
mv -f %{buildroot}%{_prefix}/lib/libsupc++.*a 32/
mv -f %{buildroot}%{_prefix}/lib/libgomp.*a 32/
ln -sf ../../../%{multilib_32_arch}-%{_vendor}-%{_target_os}/%{gcc_dir_version}/libstdc++.a 32/libstdc++.a
%endif

# Strip debug info from Fortran/ObjC/Java static libraries
strip -g `find . \( -name libgomp.a \
		    -o -name libgcc.a -o -name libgcov.a \) -a -type f`
popd
chmod 755 %{buildroot}%{_prefix}/%{_lib}/libgomp.so.1.*

mv $FULLPATH/include-fixed/syslimits.h $FULLPATH/include/syslimits.h
mv $FULLPATH/include-fixed/limits.h $FULLPATH/include/limits.h
for h in `find $FULLPATH/include -name \*.h`; do
  if grep -q 'It has been auto-edited by fixincludes from' $h; then
    rh=`grep -A2 'It has been auto-edited by fixincludes from' $h | tail -1 | sed 's|^.*"\(.*\)".*$|\1|'`
    diff -up $rh $h || :
    rm -f $h
  fi
done

cat > %{buildroot}%{_prefix}/bin/c89%{?binsuffix} <<"EOF"
#!/bin/sh
fl="-std=c89"
for opt; do
  case "$opt" in
    -ansi|-std=c89|-std=iso9899:1990) fl="";;
    -std=*) echo "`basename $0` called with non ANSI/ISO C option $opt" >&2
	    exit 1;;
  esac
done
exec gcc%{?binsuffix} $fl ${1+"$@"}
EOF
cat > %{buildroot}%{_prefix}/bin/c99%{?binsuffix} <<"EOF"
#!/bin/sh
fl="-std=c99"
for opt; do
  case "$opt" in
    -std=c99|-std=iso9899:1999) fl="";;
    -std=*) echo "`basename $0` called with non ISO C99 option $opt" >&2
	    exit 1;;
  esac
done
exec gcc%{?binsuffix} $fl ${1+"$@"}
EOF
chmod 755 %{buildroot}%{_prefix}/bin/c?9%{?binsuffix}

mkdir -p %{buildroot}%{_prefix}/sbin
#gcc -static -Os %{SOURCE1} -o %{buildroot}%{_prefix}/sbin/libgcc%{?binsuffix}_post_upgrade
#strip %{buildroot}%{_prefix}/sbin/libgcc%{?binsuffix}_post_upgrade

# Remove binaries we will not be including, so that they don't end up in
# gcc-debuginfo
rm -f %{buildroot}%{_prefix}/%{_lib}/{libffi*,libiberty.a}
rm -f $FULLEPATH/install-tools/{mkheaders,fixincl}
rm -f %{buildroot}%{_prefix}/lib/{32,64}/libiberty.a
%ifarch %{ix86} x86_64
%if !%{crossbuild}
rm -f %{buildroot}%{_prefix}/%{_lib}/libssp*
%endif
%endif

%ifarch %{multilib_64_archs}
# Remove libraries for the other arch on multilib arches
rm -f %{buildroot}%{_prefix}/lib/lib*.so*
rm -f %{buildroot}%{_prefix}/lib/lib*.a
%endif

# Rmove include-fixed
# /\/\/\
# native
%else
# cross
# \/\/\/
# additional install for cross
# remove some obsolete files
%if !%{accelerator_crossbuild}
#set -x
rm -rRf %buildroot/%{_prefix}/lib/libiberty.a
rm -rRf %buildroot/%{_prefix}/share
#set +x
%else
%if 0%{?gcc_package_version}
# Removed cross libraries
rm -rf %buildroot/%{_prefix}/%{cross_gcc_target_platform}/lib
%endif
# remove some obsolete files
rm -rRf %buildroot/%{_prefix}/lib/libiberty.a
rm -rRf %buildroot/%{_prefix}/share
# Fixed x86 dependencies
sed "s/@X86@/%{!?x64:x86}%{?x64}/g" -i %{_sourcedir}/baselibs.conf
%endif
# /\/\/\
# cross
%endif

cd ..

mkdir -p %{buildroot}%{_datadir}/license
COPYING3RUNTIME="libgcc libstdc++"
for pkg in $COPYING3RUNTIME; do
    cp gcc/COPYING3 %{buildroot}%{_datadir}/license/${pkg}%{?gcc_package_version}
    echo -e "\n\n\n\n\n" >> %{buildroot}%{_datadir}/license/${pkg}%{?gcc_package_version}
    cat COPYING.RUNTIME >> %{buildroot}%{_datadir}/license/${pkg}%{?gcc_package_version}
done

%if !%{crossbuild}
# checking and split packaging for native ...
# native
# \/\/\/

%check
%if 0
cd obj-%{gcc_target_platform}

# run the tests.
make %{?_smp_mflags} -k check ALT_CC_UNDER_TEST=gcc ALT_CXX_UNDER_TEST=g++ RUNTESTFLAGS="--target_board=unix/'{,-fstack-protector}'" || :
echo ====================TESTING=========================
( LC_ALL=C ../contrib/test_summary || : ) 2>&1 | sed -n '/^cat.*EOF/,/^EOF/{/^cat.*EOF/d;/^EOF/d;/^LAST_UPDATED:/d;p;}'
echo ====================TESTING END=====================
mkdir testlogs-%{_target_platform}-%{version}-%{release}
for i in `find . -name \*.log | grep -F testsuite/ | grep -v 'config.log\|acats\|ada'`; do
  ln $i testlogs-%{_target_platform}-%{version}-%{release}/ || :
done
tar cf - testlogs-%{_target_platform}-%{version}-%{release} | bzip2 -9c \
  | uuencode testlogs-%{_target_platform}.tar.bz2 || :
rm -rf testlogs-%{_target_platform}-%{version}-%{release}

%endif

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%post -n libgcc%{?gcc_package_version} -p /sbin/ldconfig
#%{_prefix}/sbin/libgcc%{?binsuffix}_post_upgrade
#rm -f %{_prefix}/sbin/libgcc%{?binsuffix}_post_upgrade

%postun -n libgcc%{?gcc_package_version} -p /sbin/ldconfig

%post -n libstdc++%{?gcc_package_version} -p /sbin/ldconfig

%postun -n libstdc++%{?gcc_package_version} -p /sbin/ldconfig

%post -n libgomp%{?gcc_package_version} -p /sbin/ldconfig

%postun -n libgomp%{?gcc_package_version} -p /sbin/ldconfig

%post -n libitm%{?gcc_package_version} -p /sbin/ldconfig

%postun -n libitm%{?gcc_package_version} -p /sbin/ldconfig

%post -n libasan%{?gcc_package_version} -p /sbin/ldconfig

%postun -n libasan%{?gcc_package_version} -p /sbin/ldconfig

%ifarch x86_64
%post -n libtsan%{?gcc_package_version} -p /sbin/ldconfig

%postun -n libtsan%{?gcc_package_version} -p /sbin/ldconfig

%post -n liblsan%{?gcc_package_version} -p /sbin/ldconfig

%postun -n liblsan%{?gcc_package_version} -p /sbin/ldconfig
%endif

%post -n libatomic%{?gcc_package_version} -p /sbin/ldconfig

%postun -n libatomic%{?gcc_package_version} -p /sbin/ldconfig

%post -n libubsan%{?gcc_package_version} -p /sbin/ldconfig

%postun -n libubsan%{?gcc_package_version} -p /sbin/ldconfig

%ifnarch %arm
%post -n libvtv%{?gcc_package_version} -p /sbin/ldconfig

%postun -n libvtv%{?gcc_package_version} -p /sbin/ldconfig
%endif

%files
%defattr(-,root,root,-)
%{_prefix}/bin/cc%{?binsuffix}
%{_prefix}/bin/c89%{?binsuffix}
%{_prefix}/bin/c99%{?binsuffix}
%{_prefix}/bin/gcc%{?binsuffix}
%{_prefix}/bin/gcov%{?binsuffix}
%{_prefix}/bin/gcc-ar%{?binsuffix}
%{_prefix}/bin/gcc-nm%{?binsuffix}
%{_prefix}/bin/gcc-ranlib%{?binsuffix}
%{_prefix}/bin/%{gcc_target_platform}-gcc%{?binsuffix}
%{_prefix}/bin/%{gcc_target_platform}-gcc-%{?gcc_dir_version}
%{_prefix}/bin/%{gcc_target_platform}-gcc-ar%{?binsuffix}
%{_prefix}/bin/%{gcc_target_platform}-gcc-nm%{?binsuffix}
%{_prefix}/bin/%{gcc_target_platform}-gcc-ranlib%{?binsuffix}
%{_mandir}/man1/gcc%{?binsuffix}.1*
%{_mandir}/man1/gcov%{?binsuffix}.1*
%dir %{_prefix}/lib/gcc
%dir %{_prefix}/lib/gcc/%{gcc_target_platform}
%dir %{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}
%dir %{_libexecdir}/gcc
%dir %{_libexecdir}/gcc/%{gcc_target_platform}
%dir %{_libexecdir}/gcc/%{gcc_target_platform}/%{gcc_dir_version}
%dir %{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include

# Shouldn't include all files under this fold, split to diff pkgs
#%{_libexecdir}/gcc/%{gcc_target_platform}/%{gcc_dir_version}/*
%{_libexecdir}/gcc/%{gcc_target_platform}/%{gcc_dir_version}/lto1
%{_libexecdir}/gcc/%{gcc_target_platform}/%{gcc_dir_version}/lto-wrapper
%{_libexecdir}/gcc/%{gcc_target_platform}/%{gcc_dir_version}/collect2
%{_libexecdir}/gcc/%{gcc_target_platform}/%{gcc_dir_version}/install-tools/*
%{_libexecdir}/gcc/%{gcc_target_platform}/%{gcc_dir_version}/liblto_plugin.*
%dir %{_libexecdir}/gcc/%{gcc_target_platform}/%{gcc_dir_version}/plugin
%{_libexecdir}/gcc/%{gcc_target_platform}/%{gcc_dir_version}/plugin/*

%dir %{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/plugin
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/plugin/gtype.state
%dir %{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/plugin/include
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/plugin/include/*

# Shouldn't include all files under this fold, split to diff pkgs
#%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/*
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/stddef.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/stdarg.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/stdalign.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/stdatomic.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/stdnoreturn.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/stdfix.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/varargs.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/float.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/limits.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/stdbool.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/iso646.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/syslimits.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/unwind.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/omp.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/stdint.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/stdint-gcc.h
%ifarch %{ix86} x86_64
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/mmintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/xmmintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/emmintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/pmmintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/tmmintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/ammintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/smmintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/popcntintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/nmmintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/bmmintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/wmmintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/immintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/avxintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/avx2intrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/adxintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/bmiintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/bmi2intrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/f16cintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/fmaintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/lzcntintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/x86intrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/fxsrintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/fma4intrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/xopintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/lwpintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/prfchwintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/ia32intrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/mm_malloc.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/mm3dnow.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/cpuid.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/cross-stdarg.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/ssp/ssp.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/ssp/stdio.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/ssp/string.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/ssp/unistd.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/rdseedintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/rtmintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/tbmintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/xsaveintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/xsaveoptintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/xtestintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/avx512cdintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/avx512erintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/avx512fintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/avx512pfintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/shaintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/install-tools/fixinc_list
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/install-tools/gsyslimits.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/install-tools/include/README
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/install-tools/include/limits.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/install-tools/macro_list
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/install-tools/mkheaders.conf
%endif

%dir %{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include-fixed
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include-fixed/README
%ifnarch %{arm}
#kernel 3.4 upstream removed a.out.h for arm arch
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include-fixed/linux/a.out.h
%endif
# For ARM port
%ifarch %{arm}
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/arm_acle.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/arm_neon.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/mmintrin.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/ssp/ssp.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/ssp/stdio.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/ssp/string.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/ssp/unistd.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/stdfix.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/unwind-arm-common.h
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/install-tools
%endif
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/include/sanitizer

%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/crt*.o
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libgcc.a
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libgcov.a
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libgcc_eh.a
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libgcc_s.so
%exclude %{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libgomp.spec
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libgomp.a
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libgomp.so

%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libitm.a
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libitm.so
%exclude %{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libitm.spec
%exclude %{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libsanitizer.spec
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libasan.a
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libasan.so
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libasan_preinit.o
%ifarch x86_64
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libtsan.a
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libtsan.so
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/liblsan.a
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/liblsan.so
%endif
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libatomic.a
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libatomic.so
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libubsan.a
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libubsan.so
%ifnarch %arm
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libvtv.a
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libvtv.so
%endif

%ifarch %{multilib_64_archs}
%dir %{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/32
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/32/crt*.o
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/32/libgcc.a
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/32/libgcov.a
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/32/libgcc_eh.a
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/32/libgcc_s.so
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/32/libgomp.a
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/32/libgomp.so
%endif
%dir %{_libexecdir}/getconf
%{_libexecdir}/getconf/default
%doc gcc/README*  gcc/COPYING*
%manifest gcc.manifest

%files -n cpp%{?gcc_package_version}
%defattr(-,root,root,-)
/lib/cpp%{?binsuffix}
%{_prefix}/bin/cpp%{?binsuffix}
%{_mandir}/man1/cpp%{?binsuffix}.1*
%dir %{_libexecdir}/gcc
%dir %{_libexecdir}/gcc/%{gcc_target_platform}
%dir %{_libexecdir}/gcc/%{gcc_target_platform}/%{gcc_dir_version}
%{_libexecdir}/gcc/%{gcc_target_platform}/%{gcc_dir_version}/cc1
%manifest gcc.manifest

%files -n libgcc%{?gcc_package_version}
%defattr(-,root,root,-)
/%{_lib}/libgcc_s-%{gcc_dir_version}.so.1
/%{_lib}/libgcc_s.*
/%{_libdir}/libgcc_s.*
#%{_prefix}/sbin/libgcc%{?binsuffix}_post_upgrade
%doc gcc/COPYING.LIB
%{_datadir}/license/libgcc%{?gcc_package_version}
%manifest gcc.manifest

# For ARM port
%ifarch %{arm}
%{_prefix}/%{_lib}/libssp*
%endif

%files c++
%defattr(-,root,root,-)
%ifnarch %{arm}
%{_prefix}/bin/%{gcc_target_platform}-*++%{?binsuffix}
%endif
%{_prefix}/bin/g++%{?binsuffix}
%{_prefix}/bin/c++%{?binsuffix}
%{_prefix}/bin/%{gcc_target_platform}-c++%{?binsuffix}
%{_prefix}/bin/%{gcc_target_platform}-g++%{?binsuffix}
%{_mandir}/man1/g++%{?binsuffix}.1*
%dir %{_prefix}/lib/gcc
%dir %{_prefix}/lib/gcc/%{gcc_target_platform}
%dir %{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}
%dir %{_libexecdir}/gcc
%dir %{_libexecdir}/gcc/%{gcc_target_platform}
%dir %{_libexecdir}/gcc/%{gcc_target_platform}/%{gcc_dir_version}
%{_libexecdir}/gcc/%{gcc_target_platform}/%{gcc_dir_version}/cc1plus
%ifarch %{multilib_64_archs}
%dir %{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/32
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/32/libstdc++.so
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/32/libstdc++.a
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/32/libsupc++.a
%endif
%ifarch %{multilib_64_archs}
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libstdc++.so
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libsupc++.a
%endif
%manifest gcc.manifest

%files -n libstdc++%{?gcc_package_version}
%defattr(-,root,root,-)
%{_prefix}/%{_lib}/libstdc++.*
%dir %{_datadir}/gdb
%dir %{_datadir}/gdb/auto-load
%dir %{_datadir}/gdb/auto-load/%{_prefix}
%dir %{_datadir}/gdb/auto-load/%{_prefix}/%{_lib}/
%{_datadir}/gdb/auto-load/%{_prefix}/%{_lib}/libstdc*gdb.py*
%dir %{_prefix}/share/gcc-%{gcc_dir_version}
%{_prefix}/share/gcc-%{gcc_dir_version}/python
%{_datadir}/license/libstdc++%{?gcc_package_version}
%manifest gcc.manifest

%files -n libstdc++%{?gcc_package_version}-devel
%defattr(-,root,root,-)
%dir %{_prefix}/include/c++
%dir %{_prefix}/include/c++/%{gcc_dir_version}
%{_prefix}/include/c++/%{gcc_dir_version}/[^gjos]*
%{_prefix}/include/c++/%{gcc_dir_version}/os*
%{_prefix}/include/c++/%{gcc_dir_version}/s[^u]*
%dir %{_prefix}/lib/gcc
%dir %{_prefix}/lib/gcc/%{gcc_target_platform}
%dir %{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libstdc++.a
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libsupc++.a
%ifnarch  %{multilib_64_archs}
%{_prefix}/lib/gcc/%{gcc_target_platform}/%{gcc_dir_version}/libstdc++.so
%endif

%files -n libgomp%{?gcc_package_version}
%defattr(-,root,root,-)
%{_prefix}/%{_lib}/libgomp.so.*
%manifest gcc.manifest

%files -n libitm%{?gcc_package_version}
%defattr(-,root,root,-)
%{_prefix}/%{_lib}/libitm.so.*
%manifest gcc.manifest

%files -n libasan%{?gcc_package_version}
%defattr(-,root,root,-)
%{_prefix}/%{_lib}/libasan.so.*
%manifest gcc.manifest

%ifarch x86_64
%files -n libtsan%{?gcc_package_version}
%defattr(-,root,root,-)
%{_prefix}/%{_lib}/libtsan.so.*
%manifest gcc.manifest

%files -n liblsan%{?gcc_package_version}
%defattr(-,root,root,-)
%{_prefix}/%{_lib}/liblsan.so.*
%manifest gcc.manifest
%endif

%files -n libatomic%{?gcc_package_version}
%defattr(-,root,root,-)
%{_prefix}/%{_lib}/libatomic.so.*
%manifest gcc.manifest

%files -n libubsan%{?gcc_package_version}
%defattr(-,root,root,-)
%{_prefix}/%{_lib}/libubsan.so.*
%manifest gcc.manifest

%ifnarch %arm
%files -n libvtv%{?gcc_package_version}
%defattr(-,root,root,-)
%{_prefix}/%{_lib}/libvtv.so.*
%manifest gcc.manifest
%endif

# /\/\/\
# native
%else
# cross
# \/\/\/
%files
%defattr(-,root,root,-)
%{_prefix}
%{_datadir}/license/*
%manifest gcc.manifest
# /\/\/\
# cross
%endif
