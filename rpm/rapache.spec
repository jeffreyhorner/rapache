%{!?_httpd_confdir:    %{expand: %%global _httpd_confdir %%{_sysconfdir}/httpd/conf.d}}
# /etc/httpd/conf.d with httpd < 2.4 and defined as /etc/httpd/conf.modules.d with httpd >= 2.4
# But httpd < 2.4 was a long long time ago.
%{!?_httpd_modconfdir: %{expand: %%global _httpd_modconfdir %%{_sysconfdir}/httpd/conf.modules.d}}
%{!?_httpd_moddir:     %{expand: %%global _httpd_moddir %%{_libdir}/httpd/modules}}

Name:			rapache
Version:		1.2.9
Release:		1%{?dist}
Source0:		https://github.com/jeffreyhorner/rapache/archive/v%{version}.tar.gz
License:		ASL 2.0
Summary:		The mod_R module for the Apache httpd web server
URL:			http://www.rapache.net
BuildRequires:		httpd-devel
BuildRequires:		libapreq2-devel
BuildRequires:		R-devel
BuildRequires:		make, gcc
Requires(post):		/usr/sbin/sestatus
Requires(post):		policycoreutils-python
Requires(postun):	policycoreutils-python
Requires:		httpd
Requires:		R-core

%description
rApache is a project supporting web application development using the R
statistical language and environment and the Apache web server. The current
release runs on UNIX/Linux and Mac OS X operating systems.

%prep
%setup -q

%build
%configure --with-apache2-apxs=%{_httpd_apxs}
%make_build

%install
mkdir -p %{buildroot}%{_httpd_moddir}
install -p .libs/mod_R.so %{buildroot}%{_httpd_moddir}
mkdir -p %{buildroot}%{_httpd_modconfdir}
install -p ./rpm/99-rapache.conf %{buildroot}%{_httpd_modconfdir}
mkdir -p %{buildroot}%{_httpd_confdir}
install -p ./rpm/00-rapache.conf %{buildroot}%{_httpd_confdir}
mkdir -p  %{buildroot}/var/www/html/R
install -p ./rpm/test %{buildroot}%{_localstatedir}/www/html/R/

%post
semanage fcontext -a -t httpd_modules_t '%{_httpd_moddir}/mod_R.so' 2>/dev/null || :
restorecon -R %{_httpd_moddir} || :

%postun
if [ $1 -eq 0 ]; then
	semanage fcontext -d -t httpd_modules_t '%{_httpd_moddir}/mod_R.so' 2>/dev/null || :
fi

# echo "Configuring SELinux. This takes a while..."
# Perhaps leave this to the sysadmin:
# setsebool -P httpd_setrlimit=1 httpd_can_network_connect_db=1 httpd_can_network_connect=1 httpd_can_connect_ftp=1 httpd_can_sendmail=1 || true
# Restart to load module

%files
%license LICENSE
%doc README.md
%config(noreplace) %{_httpd_confdir}/00-rapache.conf
%config(noreplace) %{_httpd_modconfdir}/99-rapache.conf
%{_httpd_moddir}/mod_R.so
%{_localstatedir}/www/html/R

%changelog
* Sat Jan 11 2020 Tom Callaway <spot@fedoraproject.org> - 1.2.9-1
- initial Fedora spec based on upstream work
