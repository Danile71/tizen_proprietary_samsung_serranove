Name:       isf
Summary:    Input Service Framework
Version:    2.4.9325
Release:    1
Group:      System Environment/Libraries
License:    LGPL-2.1+
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  edje-bin
BuildRequires:  gettext-tools
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(libprivilege-control)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(edje)
BuildRequires:  pkgconfig(x11)
%if "%{?tizen_profile_name}" != "tv"
BuildRequires:  pkgconfig(notification)
%endif
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(tts)
BuildRequires:  pkgconfig(security-server)
BuildRequires:  pkgconfig(edbus)
BuildRequires:  pkgconfig(capi-network-bluetooth)
BuildRequires:  pkgconfig(feedback)
BuildRequires:  efl-extension-devel
BuildRequires:  pkgconfig(pkgmgr-info)
BuildRequires:  pkgconfig(db-util)
BuildRequires:  pkgconfig(capi-appfw-app-control)
BuildRequires:  pkgconfig(capi-appfw-application)
BuildRequires:  capi-appfw-package-manager-devel
BuildRequires:  pkgconfig(libsystemd)
Requires(post): /sbin/ldconfig /usr/bin/vconftool
Requires(postun): /sbin/ldconfig
Requires: org.tizen.isf-kbd-mode-changer

%define _optexecdir /opt/usr/devel/usr/bin/
%define APP_PREFIX %{_prefix}/apps/org.tizen.isf-kbd-mode-changer/bin/

%description
Input Service Framewok (ISF) is an input method (IM) platform, and it has been derived from SCIM.


%package devel
Summary:    ISF header files
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
This package contains ISF header files for ISE development.


%package -n org.tizen.isf-kbd-mode-changer
Summary: isf-kbd-mode-changer
Group: Application
Requires: %{name} = %{version}-%{release}

%description -n org.tizen.isf-kbd-mode-changer
isf-kbd-mode-changer

%prep
%setup -q

%build
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"

%if "%{?tizen_profile_name}" == "mobile"
CFLAGS+=" -D_MOBILE";
CXXFLAGS+=" -D_MOBILE";
%endif

%if "%{?tizen_profile_name}" == "wearable"
CFLAGS+=" -D_WEARABLE";
CXXFLAGS+=" -D_WEARABLE";
%endif

%if "%{?tizen_profile_name}" == "tv"
CFLAGS+=" -D_TV";
CXXFLAGS+=" -D_TV";
%endif

export GC_SECTIONS_FLAGS="-fdata-sections -ffunction-sections -Wl,--gc-sections"
CFLAGS+=" -fvisibility=hidden ${GC_SECTIONS_FLAGS} "; export CFLAGS
CXXFLAGS+=" -fvisibility=hidden -fvisibility-inlines-hidden ${GC_SECTIONS_FLAGS} ";export CXXFLAGS

%autogen
%configure --disable-static \
		--disable-tray-icon --disable-filter-sctc \
		--disable-frontend-x11 \
		--disable-multiwindow-support \
		--disable-ime-embed-app
make %{?_smp_mflags}

%install
rm -rf %{buildroot}

%make_install
mkdir -p %{buildroot}%{_datadir}/license
mkdir -p %{buildroot}/opt/etc/dump.d/module.d
install -m0644 %{_builddir}/%{buildsubdir}/COPYING %{buildroot}%{_datadir}/license/%{name}
cp -af ism/dump/isf_log_dump.sh %{buildroot}/opt/etc/dump.d/module.d
mkdir -p %{buildroot}/opt/usr/dbspace
if [ ! -s %{buildroot}/opt/usr/dbspace/.ime_info.db ]; then
echo "The database file for ime will be created."
sqlite3 %{buildroot}/opt/usr/dbspace/.ime_info.db <<EOF
CREATE TABLE ime_info (appid TEXT PRIMARY KEY NOT NULL, label TEXT, pkgid TEXT, pkgtype TEXT, exec TEXT, mname TEXT, mpath TEXT, mode INTEGER, options INTEGER, is_enabled INTEGER, is_preinstalled INTEGER, has_option INTEGER, disp_lang TEXT);
EOF
fi

%post
/sbin/ldconfig
mkdir -p /etc/scim/conf
mkdir -p /opt/apps/scim/lib/scim-1.0/1.4.0/Helper
mkdir -p /opt/apps/scim/lib/scim-1.0/1.4.0/SetupUI
mkdir -p /opt/apps/scim/lib/scim-1.0/1.4.0/IMEngine
%if "%{?tizen_profile_name}" == "mobile"
mkdir -p %{_sysconfdir}/systemd/default-extra-dependencies/ignore-units.d/
ln -sf %{_libdir}/systemd/system/scim.service %{_sysconfdir}/systemd/default-extra-dependencies/ignore-units.d/
%endif


%postun -p /sbin/ldconfig


%files
%manifest %{name}.manifest
/etc/smack/accesses.d/%{name}.rule
%defattr(-,root,root,-)
%{_libdir}/systemd/system/scim.service
%{_libdir}/systemd/system/multi-user.target.wants/scim.service
%{_libdir}/systemd/system/scim.socket
%{_libdir}/systemd/system/sockets.target.wants/scim.socket
%attr(755,root,root) %{_sysconfdir}/profile.d/isf.sh
%{_sysconfdir}/scim/global
%{_sysconfdir}/scim/config
%{_datadir}/scim/isf_candidate_theme1.edj
%{_datadir}/scim/icons/*
%{_datadir}/locale/*
%{_optexecdir}/isf-demo-efl
%{_bindir}/scim
%{_bindir}/isf-log
%{_bindir}/isf-panel-efl
%{_libdir}/ecore_imf/modules/*/*/*.so
%{_libdir}/scim-1.0/1.4.0/IMEngine/socket.so
%{_libdir}/scim-1.0/1.4.0/Config/simple.so
%{_libdir}/scim-1.0/1.4.0/Config/socket.so
%{_libdir}/scim-1.0/1.4.0/FrontEnd/*.so
%{_libdir}/scim-1.0/scim-launcher
%{_libdir}/scim-1.0/scim-helper-launcher
%{_libdir}/libscim-*.so*
%{_datadir}/license/%{name}
/opt/etc/dump.d/module.d/*
%attr(660,root,app) /opt/usr/dbspace/.ime_info.db*

%files devel
%defattr(-,root,root,-)
%{_includedir}/scim-1.0/*
%{_libdir}/libscim-*.so
%{_libdir}/pkgconfig/isf.pc
%{_libdir}/pkgconfig/scim.pc

%post -n org.tizen.isf-kbd-mode-changer
mkdir -p /usr/apps/org.tizen.isf-kbd-mode-changer

%files -n org.tizen.isf-kbd-mode-changer
%manifest org.tizen.isf-kbd-mode-changer.manifest
/etc/smack/accesses.d/org.tizen.isf-kbd-mode-changer.rule
/usr/share/packages/org.tizen.isf-kbd-mode-changer.xml
%{APP_PREFIX}/*

