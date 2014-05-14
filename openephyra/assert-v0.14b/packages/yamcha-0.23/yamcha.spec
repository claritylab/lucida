%define prefix /usr
%define version 0.23
%define release 1

Summary: Yet Another Multipurpose CHunk Annotator
Name: yamcha
Version: %{version}
Release: %{release}
Copyright: LGPL
Group: local
Packager: Taku Kudoh <taku-ku@is.aist-nara.ac.jp>
Source: %{name}-%{version}.tar.gz
BuildRoot: /var/tmp/%{name}

%description
Yet Another Multipurpose CHunk Annotator
(General-Purpose Tagger with SVMs)

%package devel
Summary: Libraries and header files for YamCha
Group: Development/Libraries
Requires: TinySVM >= 0.02

%description devel
Libraries and header files for YamCha

%prep

%setup

%build
./configure --prefix=%{prefix}
make CFLAGS="$RPM_OPT_FLAGS" CXXFLAGS="$RPM_OPT_FLAGS"

%install
make prefix=$RPM_BUILD_ROOT%{prefix} install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%doc doc/*.html doc/*.css
%{prefix}/lib/*.so.*
%{prefix}/bin/*
%{prefix}/man/*/yamcha.1*
%{prefix}/libexec/*

%files devel
%defattr(-, root, root)
%{prefix}/include/*
%{prefix}/lib/*.so
%{prefix}/lib/*.a
%{prefix}/lib/*.la
