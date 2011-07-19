AM_CPPFLAGS = $(APACHE1_INCLUDES) @APR_INCLUDES@
TEST_CONFIG_SCRIPT = package Apache::TestMM; filter_args(); generate_script("t/TEST")

pkgcfgdir = `@APACHE1_APXS@ -q SYSCONFDIR`
pkglibdir = `@APACHE1_APXS@ -q LIBEXECDIR`
pkgincludedir = `@APACHE1_APXS@ -q INCLUDEDIR`/@APREQ_LIBNAME@

if MAINTAINER_MODE

APACHE1_INCLUDES = -isystem `@APACHE1_APXS@ -q INCLUDEDIR`

else

APACHE1_INCLUDES = -I`@APACHE1_APXS@ -q INCLUDEDIR`

endif


mod_apreq2_la_LDFLAGS = -export-dynamic -module -avoid-version \
                        `@APREQ_CONFIG@ --link-libtool --libs` @APR_LTFLAGS@
mod_apreq2_la_SOURCES = apreq_private_apache.h handle.c command.c


if HAVE_APACHE1

pkginclude_HEADERS = apreq_module_apache.h
pkglib_LTLIBRARIES = mod_apreq2.la

run_tests : t/TEST
	-cp -Rp .libs t
	MAKE=$(MAKE) @PERL@ t/TEST

t/TEST : Makefile.am t/TEST.PL
	@PERL@ -MApache::TestMM -e '$(TEST_CONFIG_SCRIPT)' -- -apxs @APACHE1_APXS@

test :: all check run_tests

test_clean : cmodules_clean
	-MAKE=$(MAKE) @PERL@ t/TEST -clean
	-rm -rf t/htdocs t/logs t/modules t/TEST t/core t/core.* t/cgi-bin/test_cgi t/cgi-bin/.libs t/.libs t/conf/extra.conf t/conf/ssl/ca t/conf/ssl/httpd-passphrase.pl

cmodules_clean:
	-cd c-modules && $(MAKE) clean
	-rm c-modules/Makefile c-modules/*/Makefile c-modules/apache_httpd_test.h

clean-local: test_clean


install-exec-local :
	@echo "----------------------------------------------------------------------"
	@echo "Before you can use mod_apreq2, you must ensure that an appropriate"
	@echo "\"LoadModule\" line appears in your webserver's config file:"
	@echo "$(pkgcfgdir)/httpd.conf"
	@echo 
	@echo "LoadModule apreq_module    $(pkglibdir)/mod_apreq2.so"
	@echo "----------------------------------------------------------------------"

endif
