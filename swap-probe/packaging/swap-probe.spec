Name:       swap-probe-total
Summary:    SWAP probe library
Version:    3.0
Release:    1
Group:      System/Libraries
License:    LGPL-2.1+ and MIT
Source:    %{name}_%{version}.tar.gz
%ifarch %{ix86}
BuildRequires:  emulator-yagl
%endif
# opengl-es-virtual-drv is installed as a dependency. On a real device coregl
# is actually used.
%ifarch %{arm}

%if "%{sec_product_feature_profile_wearable}" == "1"
BuildRequires: -opengl-es-virtual-drv
BuildRequires: evas
BuildRequires: opengl-es-mali400mp
%else
BuildConflicts: opengl-es-virtual-drv
BuildRequires: coregl
%endif

%endif
BuildRequires:  ecore-devel
BuildRequires:  elementary-devel
BuildRequires:  capi-appfw-application-devel
BuildRequires:  capi-system-runtime-info-devel
BuildRequires:  libXext-devel
BuildRequires:  python
Provides:  swap-probe

%description
SWAP probe is a part of data collection back-end for DA.
This library will be installed in target.

%package -n swap-probe
Provides: swap-probe
Summary:    SWAP probe library

%description  -n swap-probe
SWAP probe is a part of data collection back-end for DA.
This library will be installed in target.


%package -n swap-probe-devel
Provides: swap-probe-devel
Summary:    SWAP probe library

%description  -n swap-probe-devel
SWAP probe is a part of data collection back-end for DA.
This library will be installed in target.

%package -n swap-probe-elf
Provides: swap-probe-elf
Summary:    Swap elf parsing tool

%description -n swap-probe-elf
SWAP probe is a part of data collection back-end for DA.
This tool will be installed in target

%prep
%setup -q -n %{name}_%{version}

%build
make rmheaders
make headers
make -j
make ldheader

%install
rm -rf ${RPM_BUILD_ROOT}
mkdir -p %{buildroot}/usr/share/license
cp LICENSE.MIT %{buildroot}/usr/share/license/%{name}
cat LICENSE.LGPLv2.1+ >> %{buildroot}/usr/share/license/%{name}

mkdir -p %{buildroot}/usr/local/include
mkdir -p %{buildroot}/usr/local/bin

%make_install

%files -n swap-probe
/usr/share/license/%{name}

%manifest swap-probe.manifest
%defattr(-,root,root,-)
%{_prefix}/lib/da_probe_tizen.so
%{_prefix}/lib/libdaprobe.so

%files -n swap-probe-devel
/usr/local/include/ld_preload_*.h
/usr/local/include/x_define_api_id_list.h
/usr/local/include/app_protocol.h

%files -n swap-probe-elf
%manifest swap-probe-elf.manifest
%defattr(-,root,root,-)
/usr/local/bin/parse_elf
/usr/local/include/parse_elf.h
%{_prefix}/lib/libparserelf.so


%changelog
