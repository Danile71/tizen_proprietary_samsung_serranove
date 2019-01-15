Name:	bluetooth-sbc-codec
Summary:    SBC codec for bluetooth
Version: 1.0
Release:    4
Group:      TO_BE/FILLED_IN
License:       GPLv2 and LGPLv2+
URL:           http://www.bluez.org
Source0:       http://www.kernel.org/pub/linux/bluetooth/%{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(sndfile)

%description
SBC codec for bluetooth

%package bin
Summary:    Utility for SBC codec
Group:      TO_BE/FILLED_IN
Requires:   %{name} = %{version}-%{release}

%description bin
Development librariy for Bluetooth sbc codec

%package devel
Summary:    Development library for Bluetooth sbc codec
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Development librariy for Bluetooth sbc codec

%prep
%setup -q

%build
%reconfigure --disable-static
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}

%make_install

install -D -m 0644 COPYING %{buildroot}%{_datadir}/license/bluetooth-sbc-codec
install -D -m 0644 COPYING %{buildroot}%{_datadir}/license/bluetooth-sbc-codec-bin

%files
%{_libdir}/libsbc.so.1
%{_libdir}/libsbc.so.1.0.0
%{_datadir}/license/bluetooth-sbc-codec

%files bin
%{_bindir}/sbcdec
%{_bindir}/sbcenc
%{_bindir}/sbcinfo
%{_datadir}/license/bluetooth-sbc-codec-bin

%files devel
%{_includedir}/sbc/sbc.h
%{_libdir}/libsbc.so
%{_libdir}/pkgconfig/sbc.pc

