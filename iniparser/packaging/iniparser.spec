#sbs-git:slp/unmodified/iniparser iniparser 2.17 2a9012b9ef8ccf91d380cd8293767ab6f101830b
Name:       iniparser
Summary:    Stand-alone ini file parsing library
Version:    3.1
Release:    2
Group:      Applications/Databases
License:    MIT
URL:        http://ndevilla.free.fr/iniparser/
Source:     http://ndevilla.free.fr/iniparser/iniparser-%{version}.tar.gz
Source1001:     %{name}.manifest
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
Patch1: iniparser_getstr_add.patch

%description
iniparser is a free stand-alone ini file parsing library.
It is written in portable ANSI C and should compile anywhere.
iniparser is distributed under an MIT license.

%package devel
Summary:    Development tools for stand-alone ini file parsing library
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
This package contains the header files and development documentation
for %{name}. If you like to develop programs using %{name}, you will need
to install %{name}-devel.

%prep
%setup -q

%patch1 -p1
%build
cp %{SOURCE1001} .
LDFLAGS="-Wl,--rpath=%{_prefix}/lib -Wl,--hash-style=both -Wl,--as-needed $LDFLAGS" \
	make prefix=%{_prefix}

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}

mkdir -p $RPM_BUILD_ROOT/%{_libdir}
install -m 755 libiniparser.so.0 $RPM_BUILD_ROOT/%{_libdir}
ln -s libiniparser.so.0 $RPM_BUILD_ROOT/%{_libdir}/libiniparser.so
mkdir -p $RPM_BUILD_ROOT/%{_includedir}
install -m 755 src/*.h $RPM_BUILD_ROOT/%{_includedir}
install -m 755 src/*.h $RPM_BUILD_ROOT/%{_includedir}
mkdir -p $RPM_BUILD_ROOT/%{_libdir}/pkgconfig
cat > $RPM_BUILD_ROOT/%{_libdir}/pkgconfig/%{name}.pc <<EOF
prefix = %{_prefix}
exec_prefix = %{_exec_prefix}
libdir = %{_libdir}
includedir = %{_includedir}

Name : iniparser
Description : a free stand-alone ini file parsing library.
Version : %{version}
Libs : -L\${libdir} -liniparser
Cflags : -I\${includedir}

EOF

mkdir -p $RPM_BUILD_ROOT%{_datadir}/license
for keyword in LICENSE COPYING COPYRIGHT;
do
	for file in `find %{_builddir} -name $keyword`;
	do
		cat $file >> $RPM_BUILD_ROOT%{_datadir}/license/%{name};
		echo "";
	done;
done

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%{_datadir}/license/%{name}
%{_libdir}/*.so.*

%files devel
%{_includedir}/*.h
%{_libdir}/*.so
%{_libdir}/pkgconfig/*.pc
