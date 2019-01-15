Name:       elm-misc
Summary:    Elementary config files
Version:    1.13.31
Release:    1
Group:      TO_BE/FILLED_IN
License:    LGPLv2.1+
BuildArch:  noarch
Source0:    %{name}-%{version}.tar.gz
BuildRequires: eet-bin
BuildRequires: sec-product-features

%description
Elementary configuration files

%prep
%setup -q

%build
%if "%{?tizen_profile_name}" == "wearable"
    %if "%{?model_build_feature_formfactor}" == "circle"
     export TARGET=wearable-circle
    %else
     export TARGET=wearable
    %endif
    export ELM_PROFILE=wearable
    export SIZE=HVGA
%else
 %if "%{?tizen_profile_name}" == "mobile"
    export ELM_PROFILE=mobile
    export TARGET=mobile
    export SIZE=HD
 %else
   %if "%{?tizen_profile_name}" == "tv"
     export ELM_PROFILE=tv
     export TARGET=tv
     export SIZE=UHD
    %endif
 %endif
%endif

%if "%{?sec_product_feature_display_resolution}" == "720x1280"
	export SIZE=HD
%else
 %if "%{?sec_product_feature_display_resolution}" == "480x800"
    export SIZE=WVGA
 %endif
%endif

%if 0%{?tizen_build_binary_release_type_daily}
	%if 0%{?sec_product_feature_uifw_efl_abort_enable}
		export EFL_ABORT_ENABLE=on
	%else
		export EFL_ABORT_ENABLE=off
	%endif
%else
	export EFL_ABORT_ENABLE=off
%endif

make

%install
%if "%{?tizen_profile_name}" == "wearable"
    %if "%{?model_build_feature_formfactor}" == "circle"
     export TARGET=wearable-circle
    %else
     export TARGET=wearable
    %endif
    export ELM_PROFILE=wearable
    export SIZE=HVGA
%else
 %if "%{?tizen_profile_name}" == "mobile"
    export ELM_PROFILE=mobile
    export TARGET=mobile
    export SIZE=HD
 %else
   %if "%{?tizen_profile_name}" == "tv"
     export ELM_PROFILE=tv
     export TARGET=tv
     export SIZE=UHD
    %endif
 %endif
%endif

%if "%{?sec_product_feature_display_resolution}" == "720x1280"
    export SIZE=HD
%else
 %if "%{?sec_product_feature_display_resolution}" == "480x800"
    export SIZE=WVGA
 %endif
%endif

make install prefix=%{_prefix} DESTDIR=%{buildroot}

mkdir -p %{buildroot}%{_datadir}/license
cp %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/%{_datadir}/license/%{name}

#symbolic link default profile to current profile as if default profile is existed.
#This default profile will be used in the wm compositor desktop mode.
ln -s %{_datadir}/elementary/config/"%{?tizen_profile_name}" %{buildroot}%{_datadir}/elementary/config/default

%post
chown root:root /etc/profile.d/ecore.sh
chown root:root /etc/profile.d/edje.sh
chown root:root /etc/profile.d/eina.sh
chown root:root /etc/profile.d/elm.sh
chown root:root /etc/profile.d/evas.sh

%files
%defattr(-,root,root,-)
%{_sysconfdir}/profile.d/*
%{_datadir}/themes/*
%{_datadir}/datetimeformats/*
%{_datadir}/elementary/config/*
%{_datadir}/license/%{name}
%manifest %{name}.manifest
