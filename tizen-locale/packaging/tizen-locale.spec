Name:   tizen-locale	
Summary: carring locale information for tizen platform
Version:0.1
Release:2
License: LGPL-2.1+ and LGPL-2.1+ with exceptions
Group: 	System/Libraries

Source: %{name}-%{version}.tar.gz
Source10: generate-supported.mk
Source11: tizen-build-locale-archive
Source12: tzdata-update
Source99: LICENSES

BuildRequires: tzdata >= 2003a
Requires: coreutils
Requires: tzdata
Requires: glibc-common

%description
carring locale information for tizen platform

%prep
%setup -q 

%build
sed -ie "s#__DATADIR__#%{_datadir}#" $RPM_SOURCE_DIR/tizen-build-locale-archive
sed -ie "s#__PREFIX__#%{_prefix}#" $RPM_SOURCE_DIR/tizen-build-locale-archive

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT

mkdir -p %{buildroot}/%{_prefix}/share/license
install -m 644 %SOURCE99 $RPM_BUILD_ROOT/%{_prefix}/share/license/%{name}
mkdir -p %{buildroot}%{_sbindir}

install -m 0700 $RPM_SOURCE_DIR/tizen-build-locale-archive %{buildroot}%{_sbindir}
install -m 0700 $RPM_SOURCE_DIR/tzdata-update %{buildroot}%{_sbindir}

mkdir -p %{buildroot}/usr/lib/locale


CHARSET=UTF-8
LOCALE_DIR=%{buildroot}/usr/lib/locale
mkdir -p $LOCALE_DIR

MODEL_NAME=%{sec_build_option_product_model}
if [ -z "$MODEL_NAME" ] || [ ! -f config/${MODEL_NAME}_locale.conf ]
then
	MODEL_NAME=default
fi

REGEX=
for LOCALE in `cat config/${MODEL_NAME}_locale.conf | sed "s/#.*//"`
do
	REGEX="${REGEX}|${LOCALE}"
done
REGEX="(${REGEX#|})"

grep -E -e "SUPPORTED-LOCALES" -e "^\<$REGEX\>.*/$CHARSET" localedata/SUPPORTED > FILTERED
for LOCALE in `grep -v "SUPPORTED-LOCALES" FILTERED | cut -d '/' -f 1`
do
	I18NPATH=localedata GCONV_PATH=iconvdata localedef --quiet -c -f $CHARSET -i ${LOCALE%%.*} $LOCALE_DIR/$LOCALE
done

mkdir -p %{buildroot}/usr/share/i18n/
make -f %{SOURCE10} IN=FILTERED OUT=%{buildroot}/usr/share/i18n/SUPPORTED

%post -p /usr/sbin/tizen-build-locale-archive

%postun

%posttrans 
/bin/ls /usr/lib/locale/ | /bin/grep _ | /usr/bin/xargs -I {} /bin/rm -rf /usr/lib/locale/{}
/bin/rm -rf /usr/lib/locale/C.UTF-8
/bin/find /usr/share/locale/ -name libc.mo | /bin/grep -v en_GB | /usr/bin/xargs -I {} /bin/rm {}

%clean
rm -rf "$RPM_BUILD_ROOT"

%files
%defattr(-,root,root)
%{_prefix}/lib/locale/*
%{_prefix}/share/license/%{name}
%attr(0644,root,root) %config %{_prefix}/share/i18n/SUPPORTED
%{_sbindir}/tizen-build-locale-archive
%{_sbindir}/tzdata-update
%manifest tizen-locale.manifest
