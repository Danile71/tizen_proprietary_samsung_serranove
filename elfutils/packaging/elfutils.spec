#
# Please submit bugfixes or comments via http://bugs.meego.com/
#

%define scanf_has_m 1
%define use_zlib        1
%define use_xz          0


Name:           elfutils
Version:        0.152
Release:        1
VCS:            external/elfutils#submit/trunk/20121019.091749-3-g2ff0e357319319692c0f41bd6e70119bedc335bb
License:        GPLv2 with exceptions
Summary:        A collection of utilities and DSOs to handle compiled objects
Url:            https://fedorahosted.org/elfutils/
Group:          Development/Tools
Source:         http://fedorahosted.org/releases/e/l/elfutils/%{name}-%{version}.tar.bz2
Source1001:     elfutils.manifest
Patch1:         elfutils-robustify.patch
Patch2:         elfutils-portability.patch
Patch3:		gcc47.patch
Patch4:		gcc48.patch
Requires:       elfutils-libelf-%{_arch} = %{version}
Requires:       elfutils-libs-%{_arch} = %{version}

BuildRequires:  bison >= 1.875
BuildRequires:  bzip2
BuildRequires:  flex >= 2.5.4a

%if %{use_zlib}
BuildRequires:  bzip2-devel
BuildRequires:  pkgconfig(zlib) >= 1.2.2.3
%endif

%if %{use_xz}
BuildRequires:   pkgconfig(liblzma) 
%endif

%define _gnu %{nil}
%define _program_prefix eu-

%description
Elfutils is a collection of utilities, including ld (a linker),
nm (for listing symbols from object files), size (for listing the
section sizes of an object or archive file), strip (for discarding
symbols), readelf (to see the raw ELF file structures), and elflint
(to check for well-formed ELF files).

%package libs
Summary:        Libraries to handle compiled objects
Group:          System/Libraries
Requires:       elfutils-libelf-%{_arch} = %{version}
Provides:       elfutils-libs-%{_arch} = %{version}

%description libs
The elfutils-libs package contains libraries which implement DWARF, ELF,
and machine-specific ELF handling.  These libraries are used by the programs
in the elfutils package.  The elfutils-devel package enables building
other programs using these libraries.

%package devel
Summary:        Development libraries to handle compiled objects
Group:          Development/Libraries
Requires:       elfutils-libelf-devel-%{_arch} = %{version}
Requires:       elfutils-libs-%{_arch} = %{version}
Provides:       elfutils-devel-%{_arch} = %{version}

%description devel
The elfutils-devel package contains the libraries to create
applications for handling compiled objects.  libebl provides some
higher-level ELF access functionality.  libdw provides access to
the DWARF debugging information.  libasm provides a programmable
assembler interface.

%package devel-static
Summary:        Static archives to handle compiled objects
Group:          Development/Libraries
Requires:       elfutils-devel-%{_arch} = %{version}
Requires:       elfutils-libelf-devel-static-%{_arch} = %{version}
Provides:       elfutils-devel-static-%{_arch} = %{version}

%description devel-static
The elfutils-devel-static package contains the static archives
with the code to handle compiled objects.

%package libelf
Summary:        Library to read and write ELF files
Group:          System/Libraries
Provides:       elfutils-libelf-%{_arch} = %{version}
Obsoletes:      libelf <= 0.8.2-2

%description libelf
The elfutils-libelf package provides a DSO which allows reading and
writing ELF files on a high level.  Third party programs depend on
this package to read internals of ELF files.  The programs of the
elfutils package use it also to generate new ELF files.

%package libelf-devel
Summary:        Development support for libelf
Group:          Development/Libraries
Requires:       elfutils-libelf-%{_arch} = %{version}
Provides:       elfutils-libelf-devel-%{_arch} = %{version}
Obsoletes:      libelf-devel <= 0.8.2-2

%description libelf-devel
The elfutils-libelf-devel package contains the libraries to create
applications for handling compiled objects.  libelf allows you to
access the internals of the ELF object file format, so you can see the
different sections of an ELF file.

%package libelf-devel-static
Summary:        Static archive of libelf
Group:          Development/Libraries
Requires:       elfutils-libelf-devel-%{_arch} = %{version}
Provides:       elfutils-libelf-devel-static-%{_arch} = %{version}

%description libelf-devel-static
The elfutils-libelf-static package contains the static archive
for libelf.

%prep
%setup -q

cp %{SOURCE1001} .
%patch1 -p1 -b .robustify
%patch3 -p1
%patch4 -p1

%if !0%{?scanf_has_m}
sed -i.scanf-m -e 's/%m/%a/g' src/addr2line.c tests/line2addr.c
%endif

find . -name \*.sh ! -perm -0100 -print | xargs chmod +x

%build
# Remove -Wall from default flags.  The makefiles enable enough warnings
# themselves, and they use -Werror.  Appending -Wall defeats the cases where
# the makefiles disable some specific warnings for specific code.
RPM_OPT_FLAGS=${RPM_OPT_FLAGS/-Wall/}
RPM_OPT_FLAGS=${RPM_OPT_FLAGS/-Wunused/}

CFLAGS=${CFLAGS/-Wformat-security/}

%reconfigure CFLAGS="$CFLAGS -fexceptions" --disable-nls
make

%install
make -s install DESTDIR=%{buildroot}

chmod +x %{buildroot}%{_libdir}/lib*.so*
chmod +x %{buildroot}%{_libdir}/elfutils/lib*.so*

# XXX Nuke unpackaged files
(cd %{buildroot}
 rm -f .%{_bindir}/eu-ld
)

mkdir -p %{buildroot}/usr/share/license
cp -f COPYING %{buildroot}/usr/share/license/%{name}
cp -f COPYING %{buildroot}/usr/share/license/%{name}-libs
cp -f COPYING %{buildroot}/usr/share/license/%{name}-libelf

%clean
rm -rf %{buildroot}

%post libs -p /sbin/ldconfig

%postun libs -p /sbin/ldconfig

%post libelf -p /sbin/ldconfig

%postun libelf -p /sbin/ldconfig

%files
%defattr(-,root,root)
%doc README  COPYING
%{_bindir}/eu-addr2line
%{_bindir}/eu-ar
%{_bindir}/eu-elfcmp
%{_bindir}/eu-elflint
%{_bindir}/eu-findtextrel
%{_bindir}/eu-nm
%{_bindir}/eu-objdump
%{_bindir}/eu-ranlib
%{_bindir}/eu-readelf
%{_bindir}/eu-size
%{_bindir}/eu-strings
%{_bindir}/eu-strip
#%{_bindir}/eu-ld
%{_bindir}/eu-unstrip
%{_bindir}/eu-make-debug-archive
/usr/share/license/%{name}
%manifest %{name}.manifest

%files libs
%defattr(-,root,root)
%{_libdir}/libasm-%{version}.so
%{_libdir}/libasm.so.*
%{_libdir}/libdw-%{version}.so
%{_libdir}/libdw.so.*
%dir %{_libdir}/elfutils
%{_libdir}/elfutils/lib*.so
/usr/share/license/%{name}-libs

%files devel
%defattr(-,root,root)
%{_includedir}/dwarf.h
%dir %{_includedir}/elfutils
%{_includedir}/elfutils/elf-knowledge.h
%{_includedir}/elfutils/libasm.h
%{_includedir}/elfutils/libebl.h
%{_includedir}/elfutils/libdw.h
%{_includedir}/elfutils/libdwfl.h
%{_includedir}/elfutils/version.h
%{_libdir}/libebl.a
%{_libdir}/libasm.so
%{_libdir}/libdw.so

%files devel-static
%defattr(-,root,root)
%{_libdir}/libasm.a
%{_libdir}/libdw.a

%files libelf
%defattr(-,root,root)
%{_libdir}/libelf-%{version}.so
%{_libdir}/libelf.so.*
/usr/share/license/%{name}-libelf

%files libelf-devel
%defattr(-,root,root)
%{_includedir}/libelf.h
%{_includedir}/gelf.h
%{_includedir}/nlist.h
%{_libdir}/libelf.so

%files libelf-devel-static
%defattr(-,root,root)
%{_libdir}/libelf.a

