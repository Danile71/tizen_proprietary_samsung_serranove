Name:           python-doc
Url:            http://www.python.org/
License:        Python License ..
Group:          Development/Languages/Python
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
Summary:        Additional Package Documentation for Python.
Version:        2.7
Release:        12
%define pyver   2.7.1
BuildArch:      noarch
%define       tarname        Python-%{pyver}
%define       pyname         python
Enhances:       %{pyname}=%{pyver}
Source0:        %{tarname}.tar.bz2
Source1:        python-%{version}-docs-html.tar.bz2
Source2:        python-%{version}-docs-pdf-a4.tar.bz2
Source3:        python-%{version}-docs-pdf-letter.tar.bz2
Provides:       pyth_doc pyth_ps
Obsoletes:      pyth_doc pyth_ps

%description
Tutorial, Global Module Index, Language Reference, Library Reference,
Extending and Embedding Reference, Python/C API Reference, Documenting
Python, and Macintosh Module Reference in HTML format.



%package pdf
License:        Python License ..
Provides:       pyth_pdf
Obsoletes:      pyth_pdf
Summary:        Python PDF Documentation
Group:          Development/Languages/Python
AutoReqProv:    on

%description pdf
Tutorial, Global Module Index, Language Reference, Library Reference,
Extending and Embedding Reference, Python/C API Reference, Documenting
Python, and Macintosh Module Reference in PDF format.



Authors:
--------
    Guido van Rossum <guido@python.org>

%prep
%setup -q -n %{tarname}

%build
# nothing to do (...whistles innocently)

%install
export PDOCS=${RPM_BUILD_ROOT}%{_docdir}/%{pyname}
install -d -m 755 $PDOCS/Misc
install -d -m 755 $PDOCS/paper-a4 $PDOCS/paper-letter $PDOCS/html
tar xfj %{S:1} -C $PDOCS/
mv $PDOCS/python-%{version}-docs-html $PDOCS/html
tar xfj %{S:2} -C $PDOCS
mv $PDOCS/docs-pdf $PDOCS/paper-a4
tar xfj %{S:3} -C $PDOCS
mv $PDOCS/docs-pdf $PDOCS/paper-letter
install -c -m 644 Doc/ACKS.txt                          $PDOCS/ACKS.txt
install -c -m 644 README                        $PDOCS/README
for i in Misc/* ; do
  [ -f $i ] && install -c -m 644 $i                 $PDOCS/Misc/
done

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(644, root, root, 755)
%dir %{_docdir}/%{pyname}
%doc %{_docdir}/%{pyname}/Misc
%doc %{_docdir}/%{pyname}/html
%doc %{_docdir}/%{pyname}/ACKS.txt
%doc %{_docdir}/%{pyname}/README

%files pdf
%defattr(644, root, root, 755)
%doc %{_docdir}/%{pyname}/paper-a4
%doc %{_docdir}/%{pyname}/paper-letter

%changelog
