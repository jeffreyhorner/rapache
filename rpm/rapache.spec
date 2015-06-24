Name: rapache
Version: 1.2.7
Release: rpm0
Source: %{name}-%{version}.tar.gz
License: Apache2
Summary: RApache module for apache2 web server (mod_R)
Group: Applications/Internet
Buildroot: %{_tmppath}/%{name}-buildroot
URL: http://www.rapache.net
BuildRequires: httpd-devel
BuildRequires: libapreq2-devel
BuildRequires: R-devel
BuildRequires: make
Requires: /usr/sbin/sestatus
Requires: httpd
Requires: libapreq2
Requires: R-core

%description
First presented at DSC2005, rApache is a project supporting web application development using the R statistical language and environment and the Apache web server. The current release runs on UNIX/Linux and Mac OS X operating systems.

%prep
%setup

%build
./configure --with-apache2-apxs=/usr/bin/apxs
make

%install
#make prefix=$RPM_BUILD_ROOT install
mkdir -p %{buildroot}/etc/httpd/modules
cp .libs/mod_R.so %{buildroot}/etc/httpd/modules/
mkdir -p %{buildroot}/etc/httpd/conf.modules.d
cp ./rpm/99-rapache.conf %{buildroot}/etc/httpd/conf.modules.d/
mkdir -p %{buildroot}/etc/httpd/conf.d
cp ./rpm/00-rapache.conf %{buildroot}/etc/httpd/conf.d/
mkdir -p  %{buildroot}/var/www/html/R
cp ./rpm/test %{buildroot}/var/www/html/R/

%pre

%post
# SELinux settings
SELINUX_ENABLED=$(sestatus | grep "SELinux.status.*enabled")
if [ "$1" = 1 ] && [ "$SELINUX_ENABLED" ]; then
	chcon -t httpd_modules_t /etc/httpd/modules/mod_R.so || true
fi
# echo "Configuring SELinux. This takes a while..."
# Perhaps leave this to the sysadmin:
# setsebool -P httpd_setrlimit=1 httpd_can_network_connect_db=1 httpd_can_network_connect=1 httpd_can_connect_ftp=1 httpd_can_sendmail=1 || true
# Restart to load module
apachectl restart || true

%preun

%postun
apachectl restart || true

%files
/etc/httpd/conf.modules.d/99-rapache.conf
/etc/httpd/conf.d/00-rapache.conf
/etc/httpd/modules/mod_R.so
/var/www/html/R/test
