Name: webkit2-efl
Summary: Webkit2 EFL
Version: 123997_0.12.48.0
Release: 1
Group: Applications/Internet
License: LGPL-2.1 and BSD-2-Clause and BSD-3-Clause and Apache-1.0 and Apache-1.1
Source0: %{name}-%{version}.tar.gz

Requires(post): /sbin/ldconfig
Requires(post): xkeyboard-config
Requires(postun): /sbin/ldconfig

BuildRequires: cmake, edje-bin, embryo-bin, python, bison, flex, gperf, libjpeg-turbo-devel, perl, gettext, ruby
BuildRequires: model-build-features
BuildRequires: pkgconfig(atk)
BuildRequires: pkgconfig(cairo)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(ecore-evas)
BuildRequires: pkgconfig(ecore-file)
BuildRequires: pkgconfig(edbus)
BuildRequires: pkgconfig(edje)
BuildRequires: pkgconfig(efl-assist)
BuildRequires: pkgconfig(eina)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(evas)
BuildRequires: pkgconfig(feedback)
BuildRequires: pkgconfig(fontconfig)
BuildRequires: pkgconfig(freetype2)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(gstreamer-1.0)
BuildRequires: pkgconfig(gstreamer-plugins-base-1.0)
BuildRequires: pkgconfig(harfbuzz)
BuildRequires: pkgconfig(icu-i18n)
BuildRequires: pkgconfig(libmedia-utils)
BuildRequires: pkgconfig(libsoup-2.4)
BuildRequires: pkgconfig(openssl)
BuildRequires: pkgconfig(libxml-2.0)
BuildRequires: pkgconfig(libxslt)
BuildRequires: pkgconfig(mmutil-imgp)
BuildRequires: pkgconfig(mmutil-jpeg)
BuildRequires: pkgconfig(mm-sound)
BuildRequires: pkgconfig(sqlite3)
BuildRequires: pkgconfig(utilX)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(xt)
BuildRequires: pkgconfig(capi-appfw-app-common)
BuildRequires: pkgconfig(capi-appfw-app-control)
BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: pkgconfig(capi-network-connection)
BuildRequires: pkgconfig(capi-system-sensor)
BuildRequires: pkgconfig(capi-system-device)
BuildRequires: pkgconfig(capi-system-media-key)
BuildRequires: pkgconfig(xext)
BuildRequires: pkgconfig(gles20)
BuildRequires: pkgconfig(tts)
BuildRequires: pkgconfig(csr-framework)
BuildRequires: pkgconfig(capi-system-info)
BuildRequires: pkgconfig(capi-system-runtime-info)
BuildRequires: pkgconfig(leveldb)
BuildRequires: pkgconfig(libdri2)
BuildRequires: pkgconfig(libdrm)
BuildRequires: pkgconfig(libtbm)
BuildRequires: pkgconfig(x11)
BuildRequires: pkgconfig(xfixes)
BuildRequires: pkgconfig(libsmack)
BuildRequires: pkgconfig(capi-location-manager)
BuildRequires: pkgconfig(notification)
BuildRequires: pkgconfig(aul)
BuildRequires: pkgconfig(ewebkit2-ext)
BuildRequires: pkgconfig(capi-media-recorder)

BuildRequires: libatk-bridge-2_0-0
BuildRequires: at-spi2-atk-devel

%ifarch %{arm}
%if "%{?sec_build_project_name}" == "kirane_swa_tz" || "%{?sec_build_project_name}" == "z3a_swa_open" || "%{?sec_build_project_name}" == "z3b_swa_open" || "%{?sec_build_project_name}" == "z3c_swa_open" || "%{?sec_build_project_name}" == "z3d_swa_open"
BuildRequires: pkgconfig(UCProxySDK)
BuildRequires: pkgconfig(libwebp)
%endif
%endif

# HTML5/W3C Feature
# ARM
%ifarch %{arm}
# I586
%else
BuildRequires: pkgconfig(tts)
%endif

BuildRequires: libcap, libcap-devel, attr

# To get the Tizen version from /etc/tizen-release
Requires(post): tizen-release
Requires(post): gawk

%description
Browser Engine based on Webkit2 EFL (Shared Library)

%package devel
Summary: Webkit2 EFL
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
%description devel
Browser Engine dev library based on Webkit EFL (developement files)

%prep
%setup -q

%build

# Temporary workaround
%ifarch %{arm}
export CFLAGS="$(echo $CFLAGS | sed 's/-mfpu=[a-zA-Z0-9-]*/-mfpu=neon/g')"
export CXXFLAGS="$(echo $CXXFLAGS | sed 's/-mfpu=[a-zA-Z0-9-]*/-mfpu=neon/g')"
export FFLAGS="$(echo $FFLAGS | sed 's/-mfpu=[a-zA-Z0-9-]*/-mfpu=neon/g')"
%else
export CFLAGS="$(echo $CFLAGS | sed 's/-Wl,--as-needed//g')"
export CXXFLAGS="$(echo $CXXFLAGS | sed 's/-Wl,--as-needed//g')"
%endif

%if 0%{?nodebug}
CFLAGS=$(echo "$CFLAGS " | sed -r 's/ -g[0-3]? / /g')
CXXFLAGS=$(echo "$CXXFLAGS " | sed -r 's/ -g[0-3]? / /g')
%endif

%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif

%ifarch %{arm}
%if "%{?sec_build_project_name}" == "kirane_swa_tz" || "%{?sec_build_project_name}" == "z3a_swa_open" || "%{?sec_build_project_name}" == "z3b_swa_open" || "%{?sec_build_project_name}" == "z3c_swa_open" || "%{?sec_build_project_name}" == "z3d_swa_open"
export TIZEN_WEBKIT_FEATURE_OPTIONS="$TIZEN_WEBKIT_FEATURE_OPTIONS -DENABLE_TIZEN_COMPRESSION_PROXY:BOOL=ON"
%endif
%endif

%ifarch %{arm}
%define EFL_TARGET arm
%else
%if 0%{?simulator}
%define EFL_TARGET emulator
%else
%define EFL_TARGET i386
%endif
%endif

%if "%{EFL_TARGET}" == "emulator"
    %define _emulator ON
%else
    %define _emulator OFF
%endif

export SOURCE_DIR="$PWD"
export WEBKIT_BUILD_DIR=%{WEBKIT_BUILD_DIR}
if [ -z "$WEBKIT_BUILD_DIR" -o "{WEBKIT_BUILD_DIR}" == "${WEBKIT_BUILD_DIR#%}" ]
then
    export WEBKIT_BUILD_DIR="BuildWK2-%{EFL_TARGET}"
fi
export TIZEN_WEBKIT_EXTRA_OPTIONS=%{TIZEN_WEBKIT_EXTRA_OPTIONS}
if [ "{TIZEN_WEBKIT_EXTRA_OPTIONS}" == "${TIZEN_WEBKIT_EXTRA_OPTIONS#%}" ]
then
    export TIZEN_WEBKIT_EXTRA_OPTIONS=
fi

mkdir -p $WEBKIT_BUILD_DIR
cd $WEBKIT_BUILD_DIR

export MAKE_OPTION=%{MAKE_OPTION}
if [ "{MAKE_OPTION}" == "${MAKE_OPTION#%}" ]
then
    cmake $SOURCE_DIR -DPORT=Efl -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DENABLE_WEBKIT=Off -DENABLE_WEBKIT2=On -DENABLE_ALLINONE=On -DEFL_TARGET=%{EFL_TARGET} -DENABLE_TIZEN_EMULATOR=%{_emulator} -DDefaultTheme_RESOURCE_NAME=webkit.edj $TIZEN_WEBKIT_EXTRA_OPTIONS $TIZEN_WEBKIT_FEATURE_OPTIONS
    make %{?jobs:-j%jobs}
else
    make %{MAKE_OPTION} %{?jobs:-j%jobs}
fi

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}

export WEBKIT_BUILD_DIR=%{WEBKIT_BUILD_DIR}
if [ -z "$WEBKIT_BUILD_DIR" -o "{WEBKIT_BUILD_DIR}" == "${WEBKIT_BUILD_DIR#%}" ]
then
    export WEBKIT_BUILD_DIR="BuildWK2-%{EFL_TARGET}"
fi

cd $WEBKIT_BUILD_DIR
%make_install
mkdir -p %{buildroot}/usr/share/edje

%post
#!/bin/sh
#change file permissions
# 1. libraries
chmod 644 %{_libdir}/libewebkit2.so
# 2. Resources
chmod 644 /usr/share/edje/webkit.edj
# 3. executables
setfattr -n security.capability -v 0sAQAAAgAAAAAAAAAAAAAAAAIAAAA= %{_bindir}/WebProcess
setfattr -n security.capability -v 0sAQAAAgAAAAAAAAAAAAAAAAIAAAA= %{_bindir}/PluginProcess

%postun

%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_libdir}/*.so*
%{_bindir}/*
/usr/share/*
/usr/share/license/%{name}

%files devel
%defattr(-,root,root,-)
%{_includedir}/*
%{_libdir}/pkgconfig/*
