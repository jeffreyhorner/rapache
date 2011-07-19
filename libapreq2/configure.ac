dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT(Apache HTTP Server Request Library, 2.05-dev, apreq-dev@httpd.apache.org, libapreq2)
dnl Generate config.nice script- macro must be here at the top
dnl to avoid corruption of $0 and $@.
APR_CONFIG_NICE(config.nice)

dnl this:
dnl AC_CONFIG_AUX_DIR(build)
dnl causes problems on certain platforms
dnl aclocal.m4:817: required file `build/ltmain.sh' not found

dnl debian woody's archaic auto* tools don't set PACKAGE/VERSION correctly via 
dnl AC_INIT above, so we do it again here.
AM_INIT_AUTOMAKE(libapreq2, 2.05-dev)

AC_PROG_MAKE_SET

dnl Checks for programs.
AC_PROG_CC
AM_PROG_LIBTOOL
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_LN_S

AM_MAINTAINER_MODE

dnl Checks for header files.
AM_CONFIG_HEADER(include/apreq_config.h)
dnl Checks for typedefs, structures, and compiler characteristics.
dnl Checks for library functions.

AC_APREQ
AC_CONFIG_FILES([Makefile include/Makefile library/Makefile library/t/Makefile module/Makefile module/apache2/Makefile module/apache/Makefile glue/Makefile])
AC_CONFIG_FILES([build/doxygen.conf include/groups.dox])
AC_CONFIG_FILES([apreq2-config], [chmod +x apreq2-config])
AC_OUTPUT
