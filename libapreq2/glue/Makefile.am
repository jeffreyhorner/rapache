EXTRA_DIST = perl

dist-hook:

perl_config:
	cd perl; @PERL@ ../../build/xsbuilder.pl run

perl/Makefile: perl/xsbuilder perl/Makefile.PL
	cd perl; @PERL@ Makefile.PL -apxs @APACHE2_APXS@ @PERL_OPTS@

perl_glue: perl/Makefile
	cd perl; $(MAKE)

perl_test: perl/Makefile
	cd perl; $(MAKE) test

perl_install:
	cd perl; $(MAKE) install

perl_clean:
	-cd perl; $(MAKE) clean

if BUILD_PERL_GLUE

all-local: perl_glue

install-exec-local: perl_install

clean-local: perl_clean

test: perl_test

else

all-local:

install-exec-local:

clean-local:

test:

endif
