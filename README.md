The Rapache Project
===================

  What is it?
  -----------
  Rapache is a project dedicated to embedding the R interpreter inside
  the Apache 2.0 (and beyond) web server. It's composed of two parts:

  mod_R: the Apache 2.0 module that implements the glue to load the
  R interpreter.

  libapreq 2.0.4: an Apache sponsored project for parsing request input.
  If you don't want to compile and install this version, then you can
  specify which libapreq2 library to use during configuration.

  The Latest Version
  ------------------

  Details of the latest version can be found at the R/Apache
  project page:

	https://jeffreyhorner.github.io/rapache

  Prerequisites
  -------------
  This release has been tested under Debian Linux with Apache 2.2.4,
  R 2.5.1, and libapreq 2.0.4.  Apache2 _MUST_ be compiled with the
  prefork MPM to compile mod_R. Also, R must have been compiled with
  the --enable-R-shlib configure flag.

  Installation
  ------------
  The following is the preferred way to compile and install rapache:

    $ ./configure
    $ make
    $ make install

  configure will try to find the needed programs to compile rapache,
  but if it fails to locate them or if you have installed the
  prerequisites in non-standard places, then you can specify their
  locations with the following flags:

    --with-apache2-apxs=/path/to/apxs
    --with-R=/path/to/R
    --with-apreq2-config=/path/to/apreq2-config

  Configuration
  -------------
  Add something similar to this to the apache config file:

    LoadModule R_module /path/to/mod_R.so

    # Output R errors and warnings to the browser
    ROutputErrors

    # Displays information about rapache and R
    <Location /RApacheInfo>
        SetHandler r-info
    </Location>

    # Process all files under /path/to/brew/scripts with
    # package brew and function brew
    <Directory /path/to/brew/scripts>
        SetHandler r-script
        RHandler brew::brew
    </Directory>

    # This url will run the file /path/to/r/script.R
    <Location /made/up/url/name>
        SetHandler r-handler
        RFileHandler /path/to/r/script.R
    </Location>

  Also, libR.so _MUST_ be found in the shared library path as it is
  linked to by mod_R, RApache and the rest of the packages containing
  shared libraries. You can either set LD_LIBRARY_PATH like this:

     $ export LD_LIBRARY_PATH=`/path/to/R RHOME`/lib

  or add that path to /etc/ld.so.conf and then run ldconfig. 

  NOTE: the latest apache2 debian packages cause the web server to run
  in a very reduced environment, thus one is unable to set LD_LIBRARY_PATH
  before calling /etc/init.d/apache2. One option is to actually edit that
  file and add the LD_LIBRARY_PATH explicitly. Another is to use the apache2ctl
  scripts which are also bundled with the debian packages. Or you can add it
  to /etc/ld.so.conf.

  Documentation
  -------------
  
  Forthcoming. See https://jeffreyhorner.github.io/rapache/manual.html
  for ongoing updates.

  Licensing
  ---------

  The R/Apache source code is licensed under the Apache License Version
  2.0.  Please see the file called LICENSE.

