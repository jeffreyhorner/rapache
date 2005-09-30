AC_DEFUN([AC_APREQ], [

        AC_ARG_ENABLE(profile,
                AC_HELP_STRING([--enable-profile],[compile libapreq2 with "-pg -fprofile-arcs -ftest-coverage" for gcov/gprof]),
                [PROFILE=$enableval],[PROFILE="no"])
        AC_ARG_ENABLE(perl_glue,
                AC_HELP_STRING([--enable-perl-glue],[build perl modules Apache::Request and Apache::Cookie]),
                [PERL_GLUE=$enableval],[PERL_GLUE="no"])
        AC_ARG_WITH(perl,
                AC_HELP_STRING([--with-perl],[path to perl executable]),
                [PERL=$withval],[PERL="perl"])
        AC_ARG_WITH(apache2-apxs,
                AC_HELP_STRING([--with-apache2-apxs],[path to apache2's apxs]),
                [APACHE2_APXS=$withval],[APACHE2_APXS="apxs"])
        AC_ARG_WITH(apache2-src,
                AC_HELP_STRING([--with-apache2-src],[path to httpd source]),
                [APACHE2_SRC=$withval],[APACHE2_SRC=""])
        AC_ARG_WITH(apache2-httpd,
                AC_HELP_STRING([--with-apache2-httpd],[path to httpd binary]),
                [APACHE2_HTTPD=$withval],[APACHE2_HTTPD=""])
        AC_ARG_WITH(apr-config,
                AC_HELP_STRING([  --with-apr-config],[path to apr-*-config script]),
                [APR_CONFIG=$withval],[APR_CONFIG=""])
        AC_ARG_WITH(apu-config,
                AC_HELP_STRING([  --with-apu-config],[path to apu-*-config script]),
                [APU_CONFIG=$withval],[APU_CONFIG=""])
        AC_ARG_WITH(apache1-apxs,
                AC_HELP_STRING([--with-apache1-apxs],[path to apache1's apxs]),
                [APACHE1_APXS=$withval],[APACHE1_APXS=""])
        AC_ARG_WITH(perl-opts,
                AC_HELP_STRING([--with-perl-opts],[extra MakeMaker options]),
                [PERL_OPTS=$withval],[PERL_OPTS=""])
        AC_ARG_WITH(expat,
                AC_HELP_STRING([--with-expat],[specify expat location]),
                [EXPAT_DIR=$withval],[EXPAT_DIR=""])

        prereq_check="$PERL build/version_check.pl"

        if test -n "$APACHE2_SRC"; then
                # no apxs: must compile httpd from source

                APACHE2_SRC=`cd $APACHE2_SRC;pwd`

                AC_CHECK_FILE([$APACHE2_SRC/include/httpd.h],,
                    AC_MSG_ERROR([invalid Apache2 source directory]))

                APACHE2_INCLUDES=-I$APACHE2_SRC/include

                if test -z "$APR_CONFIG"; then
                    APR_CONFIG="$APACHE2_SRC/srclib/apr/apr-config"
                fi

                if test -z "$APU_CONFIG"; then
                    APU_CONFIG="$APACHE2_SRC/srclib/apr-util/apu-config"
                fi

                APACHE2_HTTPD=$APACHE2_SRC/httpd

        else
                # have apxs: use it

                APACHE2_INCLUDES=-I`$APACHE2_APXS -q INCLUDEDIR`

                APR_MAJOR_VERSION=`$APACHE2_APXS -q APR_VERSION 2>/dev/null | cut -d. -f 1`
                if test ${APR_MAJOR_VERSION:="0"} -eq 0; then
                    apr_config=apr-config
                    apu_config=apu-config 
                    apreq_libs="-lapr -laprutil"
                else
                    apr_config=apr-$APR_MAJOR_VERSION-config
                    apu_config=apu-$APR_MAJOR_VERSION-config
                    apreq_libs="-lapr-$APR_MAJOR_VERSION -laprutil-$APR_MAJOR_VERSION"
                fi

                if test -z "$APR_CONFIG"; then
                    APR_CONFIG=`$APACHE2_APXS -q APR_BINDIR`/$apr_config
                fi

                if test -z "$APU_CONFIG"; then
                    APU_CONFIG=`$APACHE2_APXS -q APU_BINDIR`/$apu_config
                fi

                if test -z "$APACHE2_HTTPD"; then
                    APACHE2_HTTPD=`$APACHE2_APXS -q SBINDIR`/`$APACHE2_APXS -q progname`
                fi

                if test -z "`$prereq_check apache2 $APACHE2_HTTPD`"; then
                    AC_MSG_ERROR([Bad apache2 binary ($APACHE2_HTTPD)])
                fi
        fi

        AC_CHECK_FILE([$APR_CONFIG],,
            AC_MSG_ERROR([invalid apr-config location ($APR_CONFIG)- did you forget to configure apr?]))

        if test -z "`$prereq_check apr $APR_CONFIG`"; then
            AC_MSG_ERROR([Bad libapr version])
        fi

        AC_CHECK_FILE([$APU_CONFIG],,
            AC_MSG_ERROR([invalid apu-config location ($APU_CONFIG)- did you forget to configure apr-util?]))

        if test -z "`$prereq_check apu $APU_CONFIG`"; then
            AC_MSG_ERROR([Bad libaprutil version])
        fi

        if test "x$PERL_GLUE" != "xno"; then
            AC_MSG_CHECKING(for perl)
            if test -z "`$prereq_check perl $PERL`"; then
                AC_MSG_ERROR([Bad perl version])
            fi
            AC_MSG_RESULT($PERL)

            AC_MSG_CHECKING(for ExtUtils::XSBuilder)
            if test -z "`$prereq_check ExtUtils::XSBuilder`"; then
                AC_MSG_WARN([Bad ExtUtils::XSBuilder version])
            fi
            AC_MSG_RESULT(yes)

            AC_MSG_CHECKING(for mod_perl)
            if test -z "`$prereq_check mod_perl`"; then
                AC_MSG_WARN([Bad mod_perl version])
            fi
            AC_MSG_RESULT(yes)

            AC_MSG_CHECKING(for Apache::Test)
            if test -z "`$prereq_check Apache::Test`"; then
                AC_MSG_WARN([Bad Apache::Test version])
            fi
            AC_MSG_RESULT(yes)

            AC_MSG_CHECKING(for ExtUtils::MakeMaker)
            if test -z "`$prereq_check ExtUtils::MakeMaker`"; then
                AC_MSG_WARN([Bad ExtUtils::MakeMaker version])
            fi
            AC_MSG_RESULT(yes)

        fi

        AM_CONDITIONAL(ENABLE_PROFILE, test "x$PROFILE" != "xno")
        AM_CONDITIONAL(BUILD_PERL_GLUE, test "x$PERL_GLUE" != "xno")
        AM_CONDITIONAL(HAVE_APACHE_TEST, test -n "`$prereq_check Apache::Test`")
        AM_CONDITIONAL(BUILD_HTTPD, test -n "$APACHE2_SRC")
        AM_CONDITIONAL(BUILD_APR, test "x$APR_CONFIG" = x`$APR_CONFIG --srcdir`/apr-config)
        AM_CONDITIONAL(BUILD_APU, test "x$APU_CONFIG" = x`$APU_CONFIG --srcdir`/apu-config)
        AM_CONDITIONAL(HAVE_APACHE1, test -n "$APACHE1_APXS")

        dnl Reset the default installation prefix to be the same as apu's
        ac_default_prefix="`$APU_CONFIG --prefix`"

        APR_ADDTO([APR_INCLUDES], "`$APR_CONFIG --includes`")
        APR_ADDTO([APR_INCLUDES], "`$APU_CONFIG --includes`")
        APR_LA="`$APR_CONFIG --apr-la-file`"
        APU_LA="`$APU_CONFIG --apu-la-file`"
        APR_ADDTO([APR_LTFLAGS], "`$APR_CONFIG --link-libtool`")
        APR_ADDTO([APR_LTFLAGS], "`$APU_CONFIG --link-libtool`")
        dnl perl glue/tests do not use libtool: need ld linker flags
        APR_ADDTO([APR_LIBS], "`$APU_CONFIG --libs`")
        APR_ADDTO([APR_LIBS], "`$APR_CONFIG --libs`")
        APR_ADDTO([APR_LDFLAGS], "`$APU_CONFIG --link-ld --ldflags`")
        APR_ADDTO([APR_LDFLAGS], "`$APR_CONFIG --link-ld --ldflags`")

        if test -n "$EXPAT_DIR"; then
            APR_ADDTO([APR_INCLUDES], "-I$EXPAT_DIR/include")
            APR_ADDTO([APR_LTFLAGS], "-L$EXPAT_DIR/lib")
        fi

        dnl Absolute source/build directory
        abs_srcdir=`(cd $srcdir && pwd)`
        abs_builddir=`pwd`
        top_builddir="$abs_srcdir"

        if test "$abs_builddir" != "$abs_srcdir"; then
          USE_VPATH=1
        fi

        if test "x$USE_MAINTAINER_MODE" != "xno"; then
            APR_ADDTO([CFLAGS],[
                      -Werror -Wall -Wmissing-prototypes -Wstrict-prototypes
                      -Wmissing-declarations -Wwrite-strings -Wcast-qual
                      -Wfloat-equal -Wshadow -Wpointer-arith
                      -Wbad-function-cast -Wsign-compare -Waggregate-return
                      -Wmissing-noreturn -Wmissing-format-attribute -Wpacked
                      -Wredundant-decls -Wnested-externs -Wdisabled-optimization
                      -Wno-long-long -Wendif-labels -Wcast-align -Wpacked
                      ])
                # -Wdeclaration-after-statement is only supported on gcc 3.4+
        fi

        APR_ADDTO([CPPFLAGS], "`$APR_CONFIG --cppflags`")

        get_version="$SHELL $abs_srcdir/build/get-version.sh"
        version_hdr="$abs_srcdir/include/apreq_version.h"

        # set version data

        APREQ_CONFIG="$top_builddir/apreq2-config"

        APREQ_MAJOR_VERSION=`$get_version major $version_hdr APREQ`
        APREQ_MINOR_VERSION=`$get_version minor $version_hdr APREQ`
        APREQ_PATCH_VERSION=`$get_version patch $version_hdr APREQ`
        APREQ_DOTTED_VERSION=`$get_version all  $version_hdr APREQ`

        # XXX: APR_MAJOR_VERSION doesn't yet work for static builds
        APREQ_LIBTOOL_CURRENT=`expr $APREQ_MAJOR_VERSION + $APREQ_MINOR_VERSION + $APR_MAJOR_VERSION`
        APREQ_LIBTOOL_REVISION=$APREQ_PATCH_VERSION
        APREQ_LIBTOOL_AGE=$APREQ_MINOR_VERSION

        APREQ_LIBTOOL_VERSION="$APREQ_LIBTOOL_CURRENT:$APREQ_LIBTOOL_REVISION:$APREQ_LIBTOOL_AGE"

        APREQ_LIBNAME="apreq$APREQ_MAJOR_VERSION"

        echo "lib$APREQ_LIBNAME Version: $APREQ_DOTTED_VERSION"

        AC_SUBST(APREQ_CONFIG)
        AC_SUBST(APREQ_LIBNAME)
        AC_SUBST(APREQ_LIBTOOL_VERSION)
        AC_SUBST(APREQ_MAJOR_VERSION)
        AC_SUBST(APREQ_DOTTED_VERSION)

        AC_SUBST(APACHE2_APXS)
        AC_SUBST(APACHE2_SRC)
        AC_SUBST(APACHE2_INCLUDES)
        AC_SUBST(APACHE2_HTTPD)

        AC_SUBST(APACHE1_APXS)

        AC_SUBST(APU_CONFIG)
        AC_SUBST(APR_CONFIG)
        AC_SUBST(APR_INCLUDES)
        AC_SUBST(APR_LDFLAGS)
        AC_SUBST(APR_LTFLAGS)
        AC_SUBST(APR_LIBS)
        AC_SUBST(APR_LA)
        AC_SUBST(APU_LA)

        AC_SUBST(PERL)
        AC_SUBST(PERL_OPTS)
])

dnl APR_CONFIG_NICE(filename)
dnl
dnl Saves a snapshot of the configure command-line for later reuse
dnl
AC_DEFUN([APR_CONFIG_NICE],[
  rm -f $1
  cat >$1<<EOF
#! /bin/sh
#
# Created by configure

EOF
  if test -n "$CC"; then
    echo "CC=\"$CC\"; export CC" >> $1
  fi
  if test -n "$CFLAGS"; then
    echo "CFLAGS=\"$CFLAGS\"; export CFLAGS" >> $1
  fi
  if test -n "$CPPFLAGS"; then
    echo "CPPFLAGS=\"$CPPFLAGS\"; export CPPFLAGS" >> $1
  fi
  if test -n "$LDFLAGS"; then
    echo "LDFLAGS=\"$LDFLAGS\"; export LDFLAGS" >> $1
  fi
  if test -n "$LTFLAGS"; then
    echo "LTFLAGS=\"$LTFLAGS\"; export LTFLAGS" >> $1
  fi
  if test -n "$LIBS"; then
    echo "LIBS=\"$LIBS\"; export LIBS" >> $1
  fi
  if test -n "$INCLUDES"; then
    echo "INCLUDES=\"$INCLUDES\"; export INCLUDES" >> $1
  fi
  if test -n "$NOTEST_CFLAGS"; then
    echo "NOTEST_CFLAGS=\"$NOTEST_CFLAGS\"; export NOTEST_CFLAGS" >> $1
  fi
  if test -n "$NOTEST_CPPFLAGS"; then
    echo "NOTEST_CPPFLAGS=\"$NOTEST_CPPFLAGS\"; export NOTEST_CPPFLAGS" >> $1
  fi
  if test -n "$NOTEST_LDFLAGS"; then
    echo "NOTEST_LDFLAGS=\"$NOTEST_LDFLAGS\"; export NOTEST_LDFLAGS" >> $1
  fi
  if test -n "$NOTEST_LIBS"; then
    echo "NOTEST_LIBS=\"$NOTEST_LIBS\"; export NOTEST_LIBS" >> $1
  fi

  echo [$]0 [$]ac_configure_args '"[$]@"' >> $1
  chmod +x $1
])dnl

dnl
dnl APR_ADDTO(variable, value)
dnl
dnl  Add value to variable
dnl
AC_DEFUN([APR_ADDTO],[
  if test "x$$1" = "x"; then
    echo "  setting $1 to \"$2\""
    $1="$2"
  else
    apr_addto_bugger="$2"
    for i in $apr_addto_bugger; do
      apr_addto_duplicate="0"
      for j in $$1; do
        if test "x$i" = "x$j"; then
          apr_addto_duplicate="1"
          break
        fi
      done
      if test $apr_addto_duplicate = "0"; then
        echo "  adding \"$i\" to $1"
        $1="$$1 $i"
      fi
    done
  fi
])dnl
