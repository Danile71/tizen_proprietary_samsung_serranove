Name:       shared-mime-info
Summary:    FreeDesktop.org shared MIME database and spec
Version:    0.60
Release:    26
Group:      misc
License:    GPL-2.0+
Source0:    shared-mime-info-0.60.tar.gz
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  intltool >= 0.35
BuildRequires:  libxml2

%description
FreeDesktop.org shared MIME database and spec
 This is the shared MIME-info database from the X Desktop Group. It is required
 by any program complying to the Shared MIME-Info Database spec, which is also
 included in this package.
 .
 At this time ROX and GNOME use this database. KDE is expected to follow soon, 
 and hopefully others too..

%prep
%setup -q -n %{name}-%{version}

%build
%configure --disable-static
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/usr/share/license
install  COPYING %{buildroot}/usr/share/license/%{name}

%posttrans
#remove xml files for update-mime-database
#find /usr/share/mime -name *.xml -exec rm {} \;
#for NAME in `find /usr/share/mime -mindepth 1 -type d`; do rmdir $NAME; done

%files
%defattr(-,root,root,-)
/usr/bin/update-mime-database
/usr/share/*
%manifest shared-mime-info.manifest

%changelog
* Mon Oct 20 2014 - Hyungdeuk Kim <hd3.kim@samsung.com>
- Rollback removing xml files (issue : demo binary is made by installing pkgs locally)

* Wed Jun 4 2014 - Hyungdeuk Kim <hd3.kim@samsung.com>
- Remove xml files that is not used anymore after image is created

* Tue May 27 2014 - Hyungdeuk Kim <hd3.kim@samsung.com>
- Rollback `remove po files`
- Light resource will be handled by mic script

* Fri May 23 2014 - Hyungdeuk Kim <hd3.kim@samsung.com>
- Remove unused pc file for removing pkg-config requires dependency

* Mon Apr 28 2014 - Hyungdeuk Kim <hd3.kim@samsung.com>
- Remove po files because of pkg size

* Wed Nov 20 2013 - Hyungdeuk Kim <hd3.kim@samsung.com>
- Add new mime type for S Note file(*.snb)
- Requested by bg47.kim
- Diable "0x00~1ba" match data for video/mpeg
- Issue : *.sub file is recognized as video/mpeg
- Requested by s/w verification

* Wed Nov 6 2013 - Hyungdeuk Kim <hd3.kim@samsung.com>
- Add pmd extenstion file to unknown mime type
- Add qcp extenstion file to unknown mime type
- Requested by s/w verification

* Wed Nov 6 2013 - Hyungdeuk Kim <hd3.kim@samsung.com>
- change RIFF string magic priority of audio for recognizing mime type of RIFF as video
- Requested by s/w verification

* Sat Oct 5 2013 - Hyungdeuk Kim <hd3.kim@samsung.com>
- Merge latest share mime info(ver 1.2) related ts ext file
- Requested by media server

* Thu Jun 20 2013 - Hyungdeuk Kim <hd3.kim@samsung.com>
- Merge latest shared mime info(ver 1.1) related audio/aac

* Mon May 20 2013 - Hyungdeuk Kim <hd3.kim@samsung.com>
- Merge latest shared mime info(ver 1.1) related text/vcard
- Requested by s/w verification

* Wed May 8 2013 - Hyungdeuk Kim <hd3.kim@samsung.com>
- Add text/x-vnote mime type for .vnt files
- Requested by DCM

* Tue Apr 2 2013 - Hyungdek Kim <hd3.kim@samsung.com>
- Change ldp file mimetype to application/octet-stream
- Requested by media server

* Fri Jan 4 2013 - Hyungdeuk Kim <hd3.kim@samsung.com>
- Add audio/3gpp mime type for *.3ga files

* Wed Dec 26 2012 - Hyungdeuk Kim <hd3.kim@samsung.com>
- Add new mime type for supporting elm ext files
- Requested by email team
