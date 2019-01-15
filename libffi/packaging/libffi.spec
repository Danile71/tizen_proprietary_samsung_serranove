%global multilib_arches %{ix86}  x86_64

Name:           libffi
Version:        3.1
Release:        0
Summary:        A portable foreign function interface library
License:        MIT
Url:            http://sourceware.org/libffi
Group:          Base/Toolchain
Source0:        ftp://sourceware.org/pub/libffi/libffi-%{version}.tar.gz
Source1:        ffi-multilib.h
Source2:        ffitarget-multilib.h

%description
Compilers for high level languages generate code that follow certain
conventions.  These conventions are necessary, in part, for separate
compilation to work.  One such convention is the "calling convention".
The calling convention is a set of assumptions made by the compiler
about where function arguments will be found on entry to a function.  A
calling convention also specifies where the return value for a function
is found.

Some programs may not know at the time of compilation what arguments
are to be passed to a function.  For instance, an interpreter may be
told at run-time about the number and types of arguments used to call a
given function.  `Libffi' can be used in such programs to provide a
bridge from the interpreter program to compiled code.

The `libffi' library provides a portable, high level programming
interface to various calling conventions.  This allows a programmer to
call any function specified by a call interface description at run time.

FFI stands for Foreign Function Interface.  A foreign function
interface is the popular name for the interface that allows code
written in one language to call code written in another language.  The
`libffi' library really only provides the lowest, machine dependent
layer of a fully featured foreign function interface.  A layer must
exist above `libffi' that handles type conversions for values passed
between the two languages.

%package	devel
Summary:        Development files for %{name}
Requires:       %{name} = %{version}
Requires:       pkgconfig

%description	devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.

%prep
%setup -q


%build
%reconfigure --disable-static
make %{?_smp_mflags}


%install
%make_install

# Determine generic arch target name for multilib wrapper
basearch=%{_arch}
%ifarch %{ix86}
basearch=i386
%endif

%ifarch %{multilib_arches}
# Do header file switcheroo to avoid file conflicts on systems where you
# can have both a 32- and 64-bit version of the library, and they each need
# their own correct-but-different versions of the headers to be usable.
for i in ffi ffitarget; do
  mv %{buildroot}%{_includedir}/$i.h %{buildroot}%{_includedir}/$i-${basearch}.h
done
install -m644 %{SOURCE1} %{buildroot}%{_includedir}/ffi.h
install -m644 %{SOURCE2} %{buildroot}%{_includedir}/ffitarget.h
%endif

# LICENSE
mkdir -p %{buildroot}/usr/share/license
cp -af LICENSE %{buildroot}/usr/share/license/%{name}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%{_libdir}/*.so.*
%{_datadir}/license/%{name}

%files devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/*.pc
%{_includedir}/ffi*.h
%{_libdir}/*.so
%{_mandir}/man3/*.gz
#%{_infodir}/libffi.info.gz

%changelog
