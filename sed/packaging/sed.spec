%define _bindir /bin

Name:       sed
Summary:    A GNU stream text editor
Version:    4.1.5
Release:    2
Group:      Applications/Text
License:    GPL-2.0+
URL:        http://sed.sourceforge.net/
Source0:    ftp://ftp.gnu.org/pub/gnu/sed/sed-%{version}.tar.gz
Source1001: packaging/sed.manifest


%description
The sed (Stream EDitor) editor is a stream or batch (non-interactive)
editor.  Sed takes text as input, performs an operation or set of
operations on the text and outputs the modified text.  The operations
that sed performs (substitutions, deletions, insertions, etc.) can be
specified in a script file or from the command line.




%prep
%setup -q -n %{name}-%{version}


%build
cp %{SOURCE1001} .

%configure --disable-static \
    --without-included-regex \
    --disable-nls

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%remove_docs

mkdir -p $RPM_BUILD_ROOT%{_datadir}/license
for keyword in LICENSE COPYING COPYRIGHT;
do
	for file in `find %{_builddir} -name $keyword`;
	do
		cat $file >> $RPM_BUILD_ROOT%{_datadir}/license/%{name};
		echo "";
	done;
done

%files
%manifest sed.manifest
%{_datadir}/license/%{name}
%{_bindir}/sed


