Name:       gst-omx
Summary:    GStreamer plug-in that allows communication with OpenMAX IL components
Version:    1.0.0
Release:    26
Group:      Application/Multimedia
License:    LGPLv2.1
Source0:    %{name}-%{version}.tar.gz
BuildRequires: pkgconfig(gstreamer-1.0)
BuildRequires: pkgconfig(gstreamer-plugins-base-1.0)
BuildRequires: pkgconfig(python)
BuildRequires: sec-product-features

#BuildRequires: which
BuildRequires: pkgconfig(iniparser)

BuildRequires: pkgconfig(x11)
BuildRequires: pkgconfig(mm-common)
BuildRequires: pkgconfig(libtbm)
BuildRequires: pkgconfig(libdrm)
BuildRequires: pkgconfig(dri2proto)
BuildRequires: pkgconfig(libdri2)
BuildRequires: pkgconfig(xfixes)


%description
gst-openmax is a GStreamer plug-in that allows communication with OpenMAX IL components.
Multiple OpenMAX IL implementations can be used.

%prep
%setup -q

%build
export CFLAGS+=" -DGST_EXT_XV_ENHANCEMENT"
./autogen.sh --noconfigure
%configure --prefix=/usr\
 --disable-static --with-omx-target=qcom\
%if "%{sec_product_feature_mmfw_multimedia_codec_chipset_name}" == "qualcomm_msm8916"
 --enable-msm8916\
%endif
%if 0%{?sec_product_feature_mmfw_codec_qc}
 --enable-zerocopy\
%endif

make %{?jobs:-j%jobs}


%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp COPYING %{buildroot}/usr/share/license/%{name}
mkdir -p %{buildroot}/usr/etc
cp -arf config/qcom/gstomx.conf %{buildroot}/usr/etc
%make_install

%files
%manifest gst-omx.manifest
%defattr(-,root,root,-)
%{_libdir}/gstreamer-1.0/libgstomx.so
/usr/etc/gstomx.conf
/etc/xdg/gstomx.conf
/usr/share/license/%{name}
