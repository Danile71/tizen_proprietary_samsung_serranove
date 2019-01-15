Name: libgcrypt
Version: 1.6.3
Release: 5
URL: http://www.gnupg.org/
Source0: libgcrypt-%{version}-hobbled.tar.xz

%define gcrylibdir %{_libdir}

# Technically LGPLv2.1+, but Fedora's table doesn't draw a distinction.
# Documentation and some utilities are GPLv2+ licensed. These files
# are in the devel subpackage.
License: LGPLv2+
Summary: A general-purpose cryptography library
BuildRequires: gawk, libgpg-error-devel >= 1.11, pkgconfig
#BuildRequires: fipscheck
# This is needed only when patching the .texi doc.
BuildRequires: texinfo
Group: System Environment/Libraries

%package devel
Summary: Development files for the %{name} package
License: LGPLv2+ and GPLv2+
Group: Development/Libraries
Requires(pre): /sbin/install-info
Requires(post): /sbin/install-info
Requires: libgpg-error-devel
Requires: %{name} = %{version}-%{release}

%description
Libgcrypt is a general purpose crypto library based on the code used
in GNU Privacy Guard.  This is a development version.

%description devel
Libgcrypt is a general purpose crypto library based on the code used
in GNU Privacy Guard.  This package contains files needed to develop
applications using libgcrypt.

%prep
%setup -q

%build
%configure --disable-static \
%ifarch sparc64
     --disable-asm \
%endif
     --enable-noexecstack \
     --enable-pubkey-ciphers='dsa elgamal rsa ecc' \
     --disable-O-flag-munging
sed -i -e '/^sys_lib_dlsearch_path_spec/s,/lib /usr/lib,/usr/lib /lib64 /usr/lib64 /lib,g' libtool
make %{?_smp_mflags}

# # Add generation of HMAC checksums of the final stripped binaries 
# %define __spec_install_post \
#     %{?__debug_package:%{__debug_install_post}} \
#     %{__arch_install_post} \
#     %{__os_install_post} \
# %{nil}

#    fipshmac $RPM_BUILD_ROOT%{gcrylibdir}/*.so.?? \

%install
make install DESTDIR=$RPM_BUILD_ROOT

# Change /usr/lib64 back to /usr/lib.  This saves us from having to patch the
# script to "know" that -L/usr/lib64 should be suppressed, and also removes
# a file conflict between 32- and 64-bit versions of this package.
# Also replace my_host with none.
sed -i -e 's,^libdir="/usr/lib.*"$,libdir="/usr/lib",g' $RPM_BUILD_ROOT/%{_bindir}/libgcrypt-config
sed -i -e 's,^my_host=".*"$,my_host="none",g' $RPM_BUILD_ROOT/%{_bindir}/libgcrypt-config

rm -f ${RPM_BUILD_ROOT}/%{_infodir}/dir ${RPM_BUILD_ROOT}/%{_libdir}/*.la
/sbin/ldconfig -n $RPM_BUILD_ROOT/%{_libdir}

%if "%{gcrylibdir}" != "%{_libdir}"
# Relocate the shared libraries to %{gcrylibdir}.
mkdir -p $RPM_BUILD_ROOT%{gcrylibdir}
for shlib in $RPM_BUILD_ROOT%{_libdir}/*.so* ; do
	if test -L "$shlib" ; then
		rm "$shlib"
	else
		mv "$shlib" $RPM_BUILD_ROOT%{gcrylibdir}/
	fi
done

# Add soname symlink.
/sbin/ldconfig -n $RPM_BUILD_ROOT/%{_lib}/
%endif

# Overwrite development symlinks.
pushd $RPM_BUILD_ROOT/%{gcrylibdir}
for shlib in lib*.so.?? ; do
	target=$RPM_BUILD_ROOT/%{_libdir}/`echo "$shlib" | sed -e 's,\.so.*,,g'`.so
%if "%{gcrylibdir}" != "%{_libdir}"
	shlib=%{gcrylibdir}/$shlib
%endif
	ln -sf $shlib $target
done
popd

# Create /etc/gcrypt (hardwired, not dependent on the configure invocation) so
# that _someone_ owns it.
mkdir -p -m 755 $RPM_BUILD_ROOT/etc/gcrypt

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%post devel
[ -f %{_infodir}/gcrypt.info.gz ] && \
    /sbin/install-info %{_infodir}/gcrypt.info.gz %{_infodir}/dir
exit 0

%preun devel
if [ $1 = 0 -a -f %{_infodir}/gcrypt.info.gz ]; then
    /sbin/install-info --delete %{_infodir}/gcrypt.info.gz %{_infodir}/dir
fi
exit 0

%files
%defattr(-,root,root,-)
%dir /etc/gcrypt
%{gcrylibdir}/libgcrypt.so.*
# %{gcrylibdir}/.libgcrypt.so.*.hmac
%{!?_licensedir:%global license %%doc}
%license COPYING.LIB
%doc AUTHORS NEWS THANKS

%files devel
%defattr(-,root,root,-)
%{_bindir}/%{name}-config
%{_bindir}/dumpsexp
%{_bindir}/hmac256
%{_bindir}/mpicalc
%{_includedir}/*
%{_libdir}/*.so
%{_datadir}/aclocal/*
%{_mandir}/man1/*

%{_infodir}/gcrypt.info*
%{!?_licensedir:%global license %%doc}
%license COPYING
