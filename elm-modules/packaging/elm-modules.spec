Name:       elm-modules
Summary:    The Elementary Modules
Version:    0.1.30
Release:    1
Group:      System/Libraries
License:    Flora-1.1
URL:        http://www.tizen.org
Source0:    %{name}-%{version}.tar.gz
BuildRequires: elementary-devel, eina-devel, edje-devel
BuildRequires: tts-devel, libfeedback-devel
BuildRequires: pkgconfig(mm-keysound)
BuildRequires: pkgconfig(vconf)

%description
The Elementary Modules

%prep
%setup -q

%build
export CFLAGS+=" -fvisibility=hidden -fPIC -Wall"
export LDFLAGS+=" -fvisibility=hidden -Wl,--hash-style=both -Wl,--as-needed -Wl,-z,defs"

%define DEF_SUBDIRS access_output_tts edje_feedback
for FILE in %{DEF_SUBDIRS}
do
    cd $FILE
    if [ -f autogen.sh ]
    then
        ./autogen.sh && ./configure --prefix=/usr && make
    else
        make
    fi
    cd ..
done

%install
mkdir -p %{buildroot}/%{_datadir}/license
cp %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/%{_datadir}/license/%{name}

for FILE in %{DEF_SUBDIRS}
do
        (cd $FILE && PREFIX=%{buildroot}/usr make install DESTDIR=%{buildroot})
done

%files
%defattr(-,root,root,-)
%{_libdir}/elementary/modules/*/*/*.so
%{_libdir}/elementary/modules/*/*/sounds/*
%{_libdir}/edje/modules/*/*/*.so
%{_datadir}/license/%{name}
%manifest %{name}.manifest
## Below is not needed except Machintosh
%exclude /usr/lib/elementary/modules/*/*/*.la
%exclude /usr/lib/edje/modules/*/*/*.la

