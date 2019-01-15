Name:		tizen-coreutils
Summary: 	The GNU core utilities: a set of tools commonly used in shell scripts
Version:	6.9
Release:	14
License:	GPLv2+
Group:		System Environment/Base
Url:		http://www.gnu.org/software/coreutils/
Source0:	ftp://ftp.gnu.org/gnu/%{name}/coreutils-%{version}.tar.bz2
Source1:	mktemp-1.5.tar.gz
Source1001:	%{name}.manifest
Patch1:		coreutils-futimens.patch
Patch2:		coreutils-6.9-smack.patch
Patch3:		coreutils-6.9-smack-fix.patch

Patch1001:	mktemp-1.5-build.patch
BuildRequires:	autoconf >= 2.58
BuildRequires:	automake >= 1.10
BuildRequires:	gettext findutils

Provides:	fileutils sh-utils stat textutils mktemp
Provides:	coreutils
Obsoletes:	coreutils

%description
These are the GNU core utilities.  This package is the combination of
the old GNU fileutils, sh-utils, and textutils packages.

%prep
%setup -q -b 1 -n coreutils-%{version}
%patch1 -p1 -b .futimens
%patch2 -p1 -b .smack
%patch3 -p1 -b .smack-fix

%build
cp %{SOURCE1001} .

export CFLAGS=" -fpie"
export LDFLAGS=" -pie"

pushd ../mktemp-1.5
patch -p1 < %{PATCH1001}
%configure
make
popd

%configure
make

%install

pushd ../mktemp-1.5
make bindir=%{buildroot}/bin mandir=%{buildroot}%{_mandir} install
popd

%make_install

# man pages are not installed with make install
make mandir=%{buildroot}%{_mandir} install-man

# let be compatible with old fileutils, sh-utils and textutils packages :
mkdir -p %{buildroot}{/bin,%{_bindir},%{_sbindir},/sbin}
for f in cat chgrp chmod chown cp date dd df echo false link ln ls mkdir mknod mv pwd rm rmdir sleep sync touch true uname unlink
do
    mv %{buildroot}{%{_bindir},/bin}/$f
done

# chroot was in /usr/sbin :
mv %{buildroot}{%{_bindir},%{_sbindir}}/chroot

# These come from util-linux and/or procps.
for i in hostname uptime kill ; do
    rm %{buildroot}{%{_bindir}/$i,%{_mandir}/man1/$i.1}
done

# Use hard links instead of symbolic links for LC_TIME files (bug #246729).
find %{buildroot}%{_datadir}/locale -type l | \
(while read link
 do
   target=$(readlink "$link")
   rm -f "$link"
   ln "$(dirname "$link")/$target" "$link"
 done)

mkdir -p $RPM_BUILD_ROOT%{_datadir}/license
for keyword in LICENSE COPYING COPYRIGHT;
do
	for file in `find %{_builddir} -name $keyword`;
	do
		cat $file >> $RPM_BUILD_ROOT%{_datadir}/license/%{name};
		echo "";
	done;
done

%clean
rm -rf $RPM_BUILD_ROOT

%docs_package

%files
%manifest %{name}.manifest
%doc COPYING
%{_datadir}/license/%{name}
/bin/cat
/bin/chgrp
/bin/chmod
/bin/chown
/bin/cp
/bin/date
/bin/dd
/bin/df
/bin/echo
%exclude /bin/link
/bin/ln
/bin/ls
/bin/mkdir
/bin/mknod
/bin/mktemp
/bin/mv
/bin/pwd
/bin/rm
/bin/rmdir
/bin/sleep
/bin/sync
/bin/touch
/bin/true
%exclude /bin/unlink
/bin/uname
%exclude %{_bindir}/[
%exclude %{_bindir}/base64
%{_bindir}/basename
%{_bindir}/cksum
%exclude %{_bindir}/comm
%exclude %{_bindir}/csplit
%{_bindir}/cut
%exclude %{_bindir}/dir
%exclude %{_bindir}/dircolors
%{_bindir}/dirname
%{_bindir}/du
%{_bindir}/env
%exclude %{_bindir}/expand
%{_bindir}/expr
%exclude %{_bindir}/factor
/bin/false
%exclude %{_bindir}/fmt
%exclude %{_bindir}/fold
%exclude %{_bindir}/groups
%{_bindir}/head
%exclude %{_bindir}/hostid
%{_bindir}/id
%{_bindir}/install
%exclude %{_bindir}/join
%exclude %{_bindir}/logname
%{_bindir}/md5sum
%exclude %{_bindir}/mkfifo
%{_bindir}/nice
%exclude %{_bindir}/nl
%exclude %{_bindir}/nohup
%{_bindir}/od
%exclude %{_bindir}/paste
%exclude %{_bindir}/pathchk
%exclude %{_bindir}/pinky
%exclude %{_bindir}/pr
%{_bindir}/printenv
%{_bindir}/printf
%exclude %{_bindir}/ptx
%{_bindir}/readlink
%{_bindir}/seq
%exclude %{_bindir}/sha1sum
%exclude %{_bindir}/sha224sum
%exclude %{_bindir}/sha256sum
%exclude %{_bindir}/sha384sum
%exclude %{_bindir}/sha512sum
%exclude %{_bindir}/shred
%exclude %{_bindir}/shuf
%{_bindir}/sort
%exclude %{_bindir}/split
%{_bindir}/stat
%exclude %{_bindir}/stty
%exclude %{_bindir}/sum
%{_bindir}/tac
%{_bindir}/tail
%{_bindir}/tee
%{_bindir}/test
%{_bindir}/tr
%exclude %{_bindir}/tsort
%exclude %{_bindir}/tty
%exclude %{_bindir}/unexpand
%{_bindir}/uniq
%exclude %{_bindir}/users
%exclude %{_bindir}/vdir
%{_bindir}/wc
%{_bindir}/who
%{_bindir}/whoami
%exclude %{_bindir}/yes
%{_sbindir}/chroot
%{_datadir}/locale/*
