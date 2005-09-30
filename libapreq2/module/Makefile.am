AM_CPPFLAGS = @APR_INCLUDES@
SUBDIRS = apache2
TEST_CONFIG_SCRIPT = package Apache::TestMM; filter_args(); generate_script("t/TEST")
EXTRA_DIST = t apache

noinst_PROGRAMS = test_cgi
test_cgi_LDFLAGS =  `@APREQ_CONFIG@ --link-libtool --libs` @APR_LTFLAGS@

run_tests : t/TEST
	-cp -f test_cgi t/cgi-bin
	-cp -Rp .libs t
	MAKE=$(MAKE) @PERL@ t/TEST

t/TEST : Makefile.am t/TEST.PL
	@PERL@ -MApache::TestMM -e '$(TEST_CONFIG_SCRIPT)' -- -apxs @APACHE2_APXS@

test :: all check run_tests

test_clean : cmodules_clean
	-MAKE=$(MAKE) @PERL@ t/TEST -clean
	-rm -rf t/htdocs t/logs t/modules t/TEST t/core t/core.* t/cgi-bin/test_cgi t/cgi-bin/.libs t/.libs t/conf/extra.conf t/conf/ssl/ca t/conf/ssl/httpd-passphrase.pl

cmodules_clean:
	-cd c-modules && $(MAKE) clean
	-rm c-modules/Makefile c-modules/*/Makefile c-modules/apache_httpd_test.h

clean-local: test_clean
