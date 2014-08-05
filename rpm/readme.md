# rApache rpm package

Instructions for building rApache as an rpm package. **This is all experimental**.

Make sure to run the lines below **not as root**. Except for the `sudo` lines of course.

## Fedora 20+

First setup a build environment

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
    cp rapache-1.2.6.tar.gz ~/rpmbuild/SOURCES/

    # Extract the spec file
    tar xzvf rapache-1.2.6.tar.gz rapache-1.2.6/rpm/rapache.spec
    cp rapache-1.2.6/rpm/rapache.spec ~/rpmbuild/SPECS/

Build:
    
    cd ~
    rpmbuild -ba ~/rpmbuild/SPECS/rapache.spec

If all is OK, packages are created in `~/rpmbuild/RPMS` and `~/rpmbuild/SRPMS`. To install them on your build machine:
  
    cd ~/rpmbuild/RPMS/x86_64/
    sudo rpm -i rapache-1.2.6-rpm0.x86_64.rpm
