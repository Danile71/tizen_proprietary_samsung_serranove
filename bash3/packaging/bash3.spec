%define patchleveltag 57

Version: 3.2.%{patchleveltag}
Name: bash3
Summary: The GNU Bourne Again shell
Release: 1
VCS:     external/bash#bash_4.1-slp2+s3_1-3-g35a92deccc2ab499763d1656732b42d8c433f659
Group: System/Shells
License: GPLv2+
Url: http://www.gnu.org/software/bash
Source0: ftp://ftp.gnu.org/gnu/bash/%{name}-%{version}.tar.gz
Source1001:    %{name}.manifest

# Official upstream patches
Patch1: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-001
Patch2: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-002
Patch3: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-003
Patch4: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-004
Patch5: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-005
Patch6: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-006
Patch7: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-007
Patch8: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-008
Patch9: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-009
Patch10: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-010
Patch11: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-011
Patch12: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-012
Patch13: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-013
Patch14: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-014
Patch15: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-015
Patch16: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-016
Patch17: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-017
Patch18: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-018
Patch19: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-019
Patch20: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-020
Patch21: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-021
Patch22: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-022
Patch23: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-023
Patch24: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-024
Patch25: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-025
Patch26: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-026
Patch27: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-027
Patch28: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-028
Patch29: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-029
Patch30: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-030
Patch31: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-031
Patch32: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-032
Patch33: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-033
Patch34: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-034
Patch35: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-035
Patch36: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-036
Patch37: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-037
Patch38: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-038
Patch39: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-039
Patch40: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-040
Patch41: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-041
Patch42: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-042
Patch43: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-043
Patch44: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-044
Patch45: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-045
Patch46: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-046
Patch47: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-047
Patch48: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-048
Patch49: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-049
Patch50: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-050
Patch51: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-051
Patch52: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-052
Patch53: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-053
Patch54: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-054
Patch55: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-055
Patch56: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-056
Patch57: ftp://ftp.gnu.org/gnu/bash/bash-3.2-patches/bash32-057

Provides: bash

BuildRequires: bison
BuildRequires: autoconf

%description
The GNU Bourne Again shell (Bash) is a shell or command language
interpreter that is compatible with the Bourne shell (sh). Bash
incorporates useful features from the Korn shell (ksh) and the C shell
(csh). Most sh scripts can be run by bash without modification.

%package doc
Summary: Documentation files for %{name}
Group: Development/Languages
Requires: %{name} = %{version}-%{release}

%description doc
This package contains documentation files for %{name}.

%define pkgdocdir %{_datadir}/doc/%{name}-%{version}

%prep
%setup -q
# Official upstream patches
%patch1 -p0 -b .001
%patch2 -p0 -b .002
%patch3 -p0 -b .003
%patch4 -p0 -b .004
%patch5 -p0 -b .005
%patch6 -p0 -b .006
%patch7 -p0 -b .007
%patch8 -p0 -b .008
%patch9 -p0 -b .009
%patch10 -p0 -b .010
%patch11 -p0 -b .011
%patch12 -p0 -b .012
%patch13 -p0 -b .013
%patch14 -p0 -b .014
%patch15 -p0 -b .015
%patch16 -p0 -b .016
%patch17 -p0 -b .017
%patch18 -p0 -b .018
%patch19 -p0 -b .019
%patch20 -p0 -b .020
%patch21 -p0 -b .021
%patch22 -p0 -b .022
%patch23 -p0 -b .023
%patch24 -p0 -b .024
%patch25 -p0 -b .025
%patch26 -p0 -b .026
%patch27 -p0 -b .027
%patch28 -p0 -b .028
%patch29 -p0 -b .029
%patch30 -p0 -b .030
%patch31 -p0 -b .031
%patch32 -p0 -b .032
%patch33 -p0 -b .033
%patch34 -p0 -b .034
%patch35 -p0 -b .035
%patch36 -p0 -b .036
%patch37 -p0 -b .037
%patch38 -p0 -b .038
%patch39 -p0 -b .039
%patch40 -p0 -b .040
%patch41 -p0 -b .041
%patch42 -p0 -b .042
%patch43 -p0 -b .043
%patch44 -p0 -b .044
%patch45 -p0 -b .045
%patch46 -p0 -b .046
%patch47 -p0 -b .047
%patch48 -p0 -b .048
%patch49 -p0 -b .049
%patch50 -p0 -b .050
%patch51 -p0 -b .051
%patch52 -p0 -b .052
%patch53 -p0 -b .053
%patch54 -p0 -b .054
%patch55 -p0 -b .055
%patch56 -p0 -b .056
%patch57 -p0 -b .057

%build
cp %{SOURCE1001} .
autoconf
%configure \
	--enable-largefile \
	--without-bash-malloc \
	--disable-nls

# Recycles pids is neccessary. When bash's last fork's pid was X
# and new fork's pid is also X, bash has to wait for this same pid.
# Without Recycles pids bash will not wait.
make "CPPFLAGS=-D_GNU_SOURCE -DRECYCLES_PIDS `getconf LFS_CFLAGS`"
%check
make check

%install
rm -rf $RPM_BUILD_ROOT

if [ -e autoconf ]; then
  # Yuck. We're using autoconf 2.1x.
  export PATH=.:$PATH
fi

# Fix bug #83776
perl -pi -e 's,bashref\.info,bash.info,' doc/bashref.info

make DESTDIR=$RPM_BUILD_ROOT install

mkdir -p $RPM_BUILD_ROOT/etc

# make manpages for bash builtins as per suggestion in DOC/README
pushd doc
sed -e '
/^\.SH NAME/, /\\- bash built-in commands, see \\fBbash\\fR(1)$/{
/^\.SH NAME/d
s/^bash, //
s/\\- bash built-in commands, see \\fBbash\\fR(1)$//
s/,//g
b
}
d
' builtins.1 > man.pages
for i in echo pwd test kill; do
  perl -pi -e "s,$i,,g" man.pages
  perl -pi -e "s,  , ,g" man.pages
done

install -c -m 644 builtins.1 ${RPM_BUILD_ROOT}%{_mandir}/man1/builtins.1

for i in `cat man.pages` ; do
  echo .so man1/builtins.1 > ${RPM_BUILD_ROOT}%{_mandir}/man1/$i.1
  chmod 0644 ${RPM_BUILD_ROOT}%{_mandir}/man1/$i.1
done
popd

# Link bash man page to sh so that man sh works.
ln -s bash.1 ${RPM_BUILD_ROOT}%{_mandir}/man1/sh.1

# Not for printf, true and false (conflict with coreutils)
rm -f $RPM_BUILD_ROOT/%{_mandir}/man1/printf.1
rm -f $RPM_BUILD_ROOT/%{_mandir}/man1/true.1
rm -f $RPM_BUILD_ROOT/%{_mandir}/man1/false.1

pushd $RPM_BUILD_ROOT
mkdir ./bin
mv ./usr/bin/bash ./bin
ln -sf bash ./bin/sh
rm -f .%{_infodir}/dir
popd
mkdir -p $RPM_BUILD_ROOT/etc/skel
#install -c -m644 %SOURCE1 $RPM_BUILD_ROOT/etc/skel/.bashrc
#install -c -m644 %SOURCE2 $RPM_BUILD_ROOT/etc/skel/.bash_profile
#install -c -m644 %SOURCE3 $RPM_BUILD_ROOT/etc/skel/.bash_logout
LONG_BIT=$(getconf LONG_BIT)
mv $RPM_BUILD_ROOT%{_bindir}/bashbug \
   $RPM_BUILD_ROOT%{_bindir}/bashbug-"${LONG_BIT}"

# Fix missing sh-bangs in example scripts (bug #225609).
for script in \
  examples/scripts/krand.bash \
  examples/scripts/bcsh.sh \
  examples/scripts/precedence \
  examples/scripts/shprompt
do
  cp "$script" "$script"-orig
  echo '#!/bin/bash' > "$script"
  cat "$script"-orig >> "$script"
  rm -f "$script"-orig
done

rm -rf %{buildroot}%{_bindir}/bashbug-*
chmod a-x doc/*.sh

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

# ***** bash doesn't use install-info. It's always listed in %{_infodir}/dir
# to prevent prereq loops

# post is in lua so that we can run it without any external deps.  Helps
# for bootstrapping a new install.
# Jesse Keating 2009-01-29 (code from Ignacio Vazquez-Abrams)
%post -p <lua>
bashfound = false;
shfound = false;

f = io.open("/etc/shells", "r");
if f == nil
then
  f = io.open("/etc/shells", "w");
else
  repeat
    t = f:read();
    if t == "/bin/bash"
    then
      bashfound = true;
    end
    if t == "/bin/sh"
    then
      shfound = true;
    end
  until t == nil;
end
f:close()

f = io.open("/etc/shells", "a");
if not bashfound
then
  f:write("/bin/bash\n")
end
if not shfound
then
  f:write("/bin/sh\n")
end
f:close()

%postun
if [ "$1" = 0 ]; then
    /bin/grep -v '^/bin/bash$' < /etc/shells | \
      /bin/grep -v '^/bin/sh$' > /etc/shells.new
    /bin/mv /etc/shells.new /etc/shells
fi


%docs_package

%files
%manifest %{name}.manifest
%{_datadir}/license/%{name}
/bin/sh
/bin/bash
