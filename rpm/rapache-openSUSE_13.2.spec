Name: rapache
Version: 1.2.7
Release: rpm0
Source: %{name}-%{version}.tar.gz
License: Apache2
Summary: RApache module for apache2 web server (mod_R)
Group: Applications/Internet
Buildroot: %{_tmppath}/%{name}-buildroot
URL: http://www.rapache.net
BuildRequires: apache2-devel
BuildRequires: R-base-devel
BuildRequires: libicu-devel
BuildRequires: make
Requires: libicu
Requires: apache2
Requires: apache2-prefork
Requires: R-base

%description
First presented at DSC2005, rApache is a project supporting web application development using the R statistical language and environment and the Apache web server. The current release runs on UNIX/Linux and Mac OS X operating systems.

%prep
%setup

%build
./configure
make

%install
mkdir -p $RPM_BUILD_ROOT%{_libdir}/apache2/
sed -i.bak s\,modules/mod_R.so,%{_libdir}/apache2/mod_R.so,g ./rpm/00-rapache.conf
sed -i.bak s\,/var/www/html,/srv/www/htdocs,g ./rpm/00-rapache.conf
cp .libs/mod_R.so $RPM_BUILD_ROOT%{_libdir}/apache2/
cp libapreq2/library/.libs/libapreq2.so.3 $RPM_BUILD_ROOT%{_libdir}/
mkdir -p %{buildroot}/etc/apache2/conf.d
cp ./rpm/00-rapache.conf %{buildroot}/etc/apache2/conf.d/
mkdir -p  %{buildroot}/srv/www/htdocs/R
cp ./rpm/test %{buildroot}/srv/www/htdocs/R/
export NO_BRP_CHECK_RPATH=true

%pre

%post
systemctl restart apache2.service || true

%preun

%postun
systemctl restart apache2.service || true

%files
%defattr(644,wwwrun,www,755)
/etc/apache2/conf.d/00-rapache.conf
%{_libdir}/apache2/mod_R.so
%{_libdir}/libapreq2.so.3
%dir /srv/www/htdocs/R
/srv/www/htdocs/R/test
