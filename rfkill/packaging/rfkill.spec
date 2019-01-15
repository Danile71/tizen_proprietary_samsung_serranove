%define WITH_PIE 1

#sbs-git:slp/kernel/rfkill rfkill 0.4 8e8645b934e4bed206a977a359e8c9c05a8b845a
Name:       rfkill
Summary:    Simple /dev/rfkill userspace tool
Version: 0.4
Release:    4
Group:      System/Network
License:    ISC License
Source0:    %{name}-%{version}.tar.bz2

%description
simple /dev/rfkill userspace tool

%prep
%setup -q 

%build

%if %{WITH_PIE}
export CFLAGS+=" -fPIE"
export LDFLAGS+=" -pie"
%endif

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp -a COPYING %{buildroot}/usr/share/license/%{name}
%make_install

%remove_docs

%files
%manifest rfkill.manifest
%defattr(-,root,root,-)
/usr/sbin/rfkill
/usr/share/license/rfkill

