# rApache rpm package

Instructions for building rApache as an rpm package. This has been tested on Fedora 20 and CentOS 6.

## Prepare

The standard Redhat repositories do not include R, but backports from Fedora are available through EPEL. If you are running RHEL or CentOS, you need to enable the EPEL repository. On Fedora you can skip this step.

On RHEL/CentOS 6:

    sudo su -c 'rpm -Uvh https://dl.fedoraproject.org/pub/epel/6/i386/epel-release-6-8.noarch.rpm'

On RHEL/CentOS 7:

    sudo su -c 'rpm -Uvh https://dl.fedoraproject.org/pub/epel/beta/7/x86_64/epel-release-7-0.2.noarch.rpm'

The `epel-release` rpm version changes from time to time. Find the current version on the website EPEL if you get an error. See also [this FAQ](https://fedoraproject.org/wiki/EPEL/FAQ#How_can_I_install_the_packages_from_the_EPEL_software_repository.3F).

## Building on Fedora / RHEL / CentOS

Important: make sure to run the lines in this section **not as root**. Except for the `sudo` lines of course.

First setup a build environment:

    # rpm build dependencies
    sudo yum install rpm-build

    # rApache build dependencies
    sudo yum install make wget httpd-devel libapreq2-devel R-devel

    # Build directories
    mkdir -p ~/rpmbuild/SOURCES
    mkdir -p ~/rpmbuild/SPECS

Download sources:

    # Get the rapache source code
    wget https://github.com/jeffreyhorner/rapache/archive/v1.2.6.tar.gz -O rapache-1.2.6.tar.gz
    tar xzvf rapache-1.2.6.tar.gz rapache-1.2.6/rpm/rapache.spec --strip-components 2

    # Move to build dirs
    mv -f rapache-1.2.6.tar.gz ~/rpmbuild/SOURCES/
    mv -f rapache.spec ~/rpmbuild/SPECS/

Build:

    cd ~
    rpmbuild -ba ~/rpmbuild/SPECS/rapache.spec

## Installing rApache

If all is OK, packages are created in `~/rpmbuild/RPMS` and `~/rpmbuild/SRPMS`. To install them on your build machine:

    cd ~/rpmbuild/RPMS/x86_64/
    sudo rpm -i rapache-1.2.6-rpm0.x86_64.rpm

The package includes a test script in `/var/www/html/R/test` which prints some random numers. Try if it works:

    curl http://localhost/R/test

Also try to open http://localhost/RApacheInfo in your browser.

## Debugging SELinux

If you get permission denied error when (re)starting httpd or accessing your web server, the problem is most likely SELinux. SELinux can be disabled by editing `/etc/selinux/config` and then rebooting.

If you don't want to disable SELinux, you will have to customize the security profiles for your needs. Have a look at `/var/log/messages` and `/var/log/audit/audit.log`. Installing the `setroubleshoot` and `setroubleshoot-server` packages results in better logging. The [selinux_httpd man page](http://linux.die.net/man/8/httpd_selinux) lists important booleans that you might want to turn on. See also [this help page](https://docs.fedoraproject.org/en-US/Fedora/19/html/Security_Guide/sect-Managing_Confined_Services-The_Apache_HTTP_Server.html).

## Firewall

On most standard installations of RHEL and CentOS, the default firewall configuration is to block HTTP/HTTPS traffic from external hosts. To open port 80 (HTTP) use something like:

    sudo iptables -I INPUT -p tcp --dport 80 -j ACCEPT
    sudo service iptables save

Google is your friend.
