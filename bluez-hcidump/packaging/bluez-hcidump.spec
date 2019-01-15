Name:       bluez-hcidump
Summary:    Analyses Bluetooth HCI packets
Version:    2.4
Release:    3
Source0:     %{name}-%{version}.tar.gz
Group:      Applications/System
URL:        http://www.bluez.org
License:    GPLv2+
BuildRequires:  pkgconfig(bluez)

%description
The hcidump utility allows the monitoring of Bluetooth activity.
It provides a disassembly of the Bluetooth traffic and can display
packets from higher level protocols such as RFCOMM, SDP and BNEP.
.
hcidump is part of the BlueZ Bluetooth Linux project.  For more information
see http://www.bluez.org .


%prep
%setup -q 


%build
%reconfigure --prefix=%{_prefix}

make 

%install
rm -rf %{buildroot}
%make_install

install -D -m 0644 COPYING %{buildroot}%{_datadir}/license/bluez-hcidump

%files
%manifest bluez-hcidump.manifest
%defattr(-,root,root,-)
%{_sbindir}/hcidump
%{_datadir}/license/bluez-hcidump
