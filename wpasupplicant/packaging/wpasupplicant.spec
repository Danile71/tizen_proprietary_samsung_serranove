Name:		wpasupplicant
Summary:	Support for WPA and WPA2
Version:	2.0.167
Release:	1
Group:		System/Network
License:	BSD license
Source0:	%{name}-%{version}.tar.gz

%description
WPA and WPA2 are methods for securing wireless networks, the former
using IEEE 802.1X, and the latter using IEEE 802.11i. This software
provides key negotiation with the WPA Authenticator, and controls
association with IEEE 802.11i networks.

%prep
%setup -q


%build

%install
#License
mkdir -p %{buildroot}%{_datadir}/license
cp libnl-headers/README-LGPL %{buildroot}%{_datadir}/license/libnl-headers

%post

%files
%{_datadir}/license/libnl-headers
