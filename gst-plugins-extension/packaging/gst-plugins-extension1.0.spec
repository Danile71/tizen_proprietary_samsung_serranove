Name:       gst-plugins-extension1.0
Summary:    GStreamer extra plugins
Version:    0.0.3
Release:    8
Group:      TO_BE/FILLED_IN
License:    TO_BE/FILLED_IN
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  sec-product-features
BuildRequires:  prelink
BuildRequires:  pkgconfig(gstreamer-audio-1.0)
BuildRequires:  pkgconfig(gstreamer-video-1.0)
BuildRequires:  pkgconfig(gstreamer-plugins-base-1.0)
BuildRequires:  pkgconfig(gstreamer-1.0)
#BuildRequires:  pkgconfig(mobilebae)
BuildRequires:  pkgconfig(vconf)
#BuildRequires:  pkgconfig(mm-ta)
BuildRequires:  pkgconfig(camsrcjpegenc)
BuildRequires:  pkgconfig(libdri2)
BuildRequires:  pkgconfig(libdrm)
BuildRequires:  pkgconfig(pango)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(xext)
BuildRequires:  pkgconfig(xv)
BuildRequires:  pkgconfig(xdamage)
BuildRequires:  pkgconfig(libtbm)
BuildRequires:  pkgconfig(dri2proto)
BuildRequires:  pkgconfig(xfixes)
BuildRequires:  pkgconfig(iniparser)
BuildRequires:  pkgconfig(libcrypto)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(ecore-x)
BuildRequires:  pkgconfig(evas)
%ifarch %{arm}
#BuildRequires:  pkgconfig(mm-sound)
BuildRequires:  pkgconfig(vtstack)
BuildRequires:  pkgconfig(image-filter)
%if ! 0%{?sec_product_feature_profile_lite}
BuildRequires:  pkgconfig(opengl-es-20)
BuildRequires:  pkgconfig(native-buffer)
BuildRequires:  pkgconfig(native-buffer-pool)
BuildRequires:  pkgconfig(mmutil-jpeg)
%endif
#BuildRequires:  pkgconfig(mm-wfd-common)
#BuildRequires:  pkgconfig(mm-wfd-hal)

%endif

%description
Description: GStreamer extra pluginsi

%package -n gstreamer1.0-plugins-extension
Summary:    GStreamer extra plugins (common)
Group:      TO_BE/FILLED_IN

%description -n gstreamer1.0-plugins-extension
Description: GStreamer extra plugins (common)

%ifarch {%arm}
%package -n gstreamer1.0-plugins-extension-voicecalldata
Summary:    GStreamer extra plugins (voicecalldata)
Group:      TO_BE/FILLED_IN

%description -n gstreamer1.0-plugins-extension-voicecalldata
Description: GStreamer extra plugins (voicecalldata)
%endif

%package -n gstreamer1.0-plugins-extension-submux
Summary:    GStreamer extra plugins (submux)
Group:      TO_BE/FILLED_IN

%description -n gstreamer1.0-plugins-extension-submux
Description: GStreamer extra plugins (submux)

%package -n gstreamer1.0-plugins-extension-evastextoverlay
Summary:    GStreamer extra plugins (evastextoverlay)
Group:      TO_BE/FILLED_IN

%description -n gstreamer1.0-plugins-extension-evastextoverlay
Description: GStreamer extra plugins (evastextoverlay)

%ifarch %{arm}
%if 0%{?sec_product_feature_mmfw_gl_effect}
%package -n gstreamer1.0-plugins-extension-pip
Summary:    GStreamer extra plugins (pip)
Group:      TO_BE/FILLED_IN

%description -n gstreamer1.0-plugins-extension-pip
Description: GStreamer extra plugins (pip)
%endif
%endif

%prep
%setup -q

%build
./autogen.sh
CFLAGS=" %{optflags} -DGST_EXT_XV_ENHANCEMENT -DGST_EXT_TIME_ANALYSIS -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" "; export CFLAGS
%configure \
%ifarch %{arm}
    --disable-i386 \
    --disable-wfd-extended-features     \
    --disable-ext-wfdrtspsrc           \
%if ! 0%{?sec_product_feature_mmfw_gl_effect}
    --disable-ext-pip                  \
%endif
	--disable-ext-videomute            \
    --disable-ext-voicedatasrcbin      \
    --disable-ext-subrender           \
    --disable-ext-multisession-httpsrc

%else
    --enable-i386                      \
    --disable-ext-secrtspsrc           \
    --disable-ext-sechttpsrc           \
    --disable-ext-savsimgp             \
    --disable-ext-voicedatasrcbin      \
    --disable-ext-pip                  \
%endif

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp COPYING %{buildroot}/usr/share/license/gstreamer1.0-plugins-extension
%ifarch %{arm}
cp LICENSE.MIT %{buildroot}/usr/share/license/gstreamer1.0-plugins-extension-voicecalldata
%if 0%{?sec_product_feature_mmfw_gl_effect}
cp LICENSE.MIT %{buildroot}/usr/share/license/gstreamer1.0-plugins-extension-pip
%endif
%endif
#cp LICENSE.MIT %{buildroot}/usr/share/license/gstreamer1.0-plugins-extension-midi
%make_install

mkdir -p %{buildroot}/usr/etc/

%if ! 0%{?sec_product_feature_profile_lite}
%ifarch %{arm}
mkdir -p %{buildroot}/usr/etc/
%endif
%endif

%files -n gstreamer1.0-plugins-extension
%manifest gstreamer1.0-plugins-extension.manifest
%defattr(-,root,root,-)
#%{_libdir}/gstreamer-1.0/libgstvideomute.so
%{_libdir}/gstreamer-1.0/libgstimagereader.so
#%{_libdir}/gstreamer-1.0/libgstmshttpsrc.so
#%{_libdir}/gstreamer-1.0/libgstsubrender.so
/usr/share/license/gstreamer1.0-plugins-extension


%ifarch %{arm}
%files -n gstreamer1.0-plugins-extension-voicecalldata
%manifest gstreamer1.0-plugins-extension-voicecalldata.manifest
%defattr(-,root,root,-)
#Fixme - GST1.0
%if 0
%{_libdir}/gstreamer-1.0/libgstaudiosplitter.so
%{_libdir}/gstreamer-1.0/libgstvoicedatasrcbin.so
%{_libdir}/gstreamer-1.0/libgstsocketsrc.so
%endif
/usr/share/license/gstreamer1.0-plugins-extension-voicecalldata
%endif

%files -n gstreamer1.0-plugins-extension-submux
%manifest gstreamer1.0-plugins-extension-submux.manifest
%defattr(-,root,root,-)
%{_libdir}/gstreamer-1.0/libgstsubmux.so

%files -n gstreamer1.0-plugins-extension-evastextoverlay
%manifest gstreamer1.0-plugins-extension-evastextoverlay.manifest
%defattr(-,root,root,-)
%{_libdir}/gstreamer-1.0/libgstevastextoverlay.so

%ifarch %{arm}
%if 0%{?sec_product_feature_mmfw_gl_effect}
%files -n gstreamer1.0-plugins-extension-pip
%manifest gstreamer1.0-plugins-extension-pip.manifest
%defattr(-,root,root,-)
%{_libdir}/gstreamer-1.0/libgstpip.so
/usr/share/license/gstreamer1.0-plugins-extension-pip
%endif
%endif
