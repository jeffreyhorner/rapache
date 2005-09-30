#   The following is a trick to get CPAN clients to follow prerequisites:
#
#    PREREQ_PM
#

# --- MakeMaker post_initialize section:


AUTOMAKE_OPTIONS = foreign
SUBDIRS = . include library module glue
EXTRA_DIST = LICENSE NOTICE README INSTALL CHANGES FAQ.pod MANIFEST.SKIP Makefile.PL buildconf build win32 docs
APU_SRCDIR=`@APU_CONFIG@ --srcdir`
APR_SRCDIR=`@APR_CONFIG@ --srcdir`
APU_DOX = (cd $(APU_SRCDIR); cat docs/doxygen.conf - | doxygen -)
APR_DOX = (cd $(APR_SRCDIR); cat docs/doxygen.conf - | doxygen -)
SED_BODY_TAG= s(^[\S\s]+<body.+|</body>[\S\s]+$$)()gi, s(href="/Apache(?:/\w+)*/([^/]+).html")(href="group__apreq__xs__\L$$1.html")g
EUM=ExtUtils::Manifest
POD_DIR=glue/perl/lib/Apache2
bin_SCRIPTS = apreq2-config
CLEANFILES = $(bin_SCRIPTS)

if BUILD_PERL_GLUE
PERL_TEST = perl_test
else
PERL_TEST = 
endif

release: reconfig docs_clean docs distdir
	@PERL@ build/version_check.pl > $(distdir)/PREREQUISITES
	@PERL@ build/version_check.pl -version=@PACKAGE_VERSION@ > $(distdir)/META.yml
	echo MANIFEST > $(distdir)/MANIFEST
	@cd $(distdir); @PERL@ -M$(EUM) -e "$(EUM)::mkmanifest"
	@PERL@ -ple '$$_="$(distdir)/$$_"' $(distdir)/MANIFEST | tar -czf $(distdir).tar.gz -T -
	rm -rf $(distdir)
	@echo "Made $(distdir).tar.gz"

release_test:
	-rm -rf $(distdir)
	tar xzvf $(distdir).tar.gz
	@cd $(distdir); @PERL@ -M$(EUM) -e "die 'Bogus MANIFEST' if $(EUM)::manicheck"
	@cd $(distdir); @PERL@ Makefile.PL -apxs @APACHE2_APXS@ && $(MAKE) test
	rm -rf $(distdir)
	@echo "$(distdir).tar.gz is kosher."

reconfig:
	./buildconf --with-automake=automake-1.6 --with-aclocal=aclocal-1.6
	./config.nice

docs_clean:
	-rm -rf docs

docs:	pod2html build/doxygen.conf apr_tags docs/html/feather.gif
	@PERL@ build/version_check.pl doxygen
	doxygen build/doxygen.conf

docs_install: docs
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)
	cp -a docs $(DESTDIR)$(pkgdatadir)

%.html: $(POD_DIR)/%.pm Makefile.am 
	pod2html < $< | perl -0777 -pe '$(SED_BODY_TAG)' > $@

pod2html :: Request.html Cookie.html Upload.html FAQ.html

FAQ.html: Makefile.am FAQ.pod
	pod2html < FAQ.pod | perl -0777 -pe '$(SED_BODY_TAG)' > FAQ.html

$(POD_DIR)/%.t: $(POD_DIR)/%.pod
	(cd glue/perl ; pod2test docs/$*.pod docs/$*.t)

pod2test :: $(POD_DIR)/Request.t $(POD_DIR)/Cookie.t $(POD_DIR)/Upload.t $(POD_DIR)/Error.t $(POD_DIR)/Table.t

apr_tags: docs/apr.tag docs/apu.tag

docs/html/feather.gif:
	-mkdir docs
	-mkdir docs/html
	(cd docs/html; wget http://httpd.apache.org/docs-2.0/images/feather.gif)

docs/apu.tag:
	-mkdir docs
	echo GENERATE_TAGFILE=`pwd`/docs/apu.tag | $(APU_DOX)

docs/apr.tag:
	-mkdir docs
	echo GENERATE_TAGFILE=`pwd`/docs/apr.tag | $(APR_DOX)

test: all library_test module_test $(PERL_TEST)

module_test:
	cd module; $(MAKE) test

library_test:
	cd library; $(MAKE) test

perl_install:
	cd glue/perl; $(MAKE) install

perl_test:
	cd glue/perl; $(MAKE) test

perl_glue:
	cd glue/perl; @PERL@ ../../build/xsbuilder.pl run
	cd glue/perl; @PERL@ Makefile.PL -apxs @APACHE2_APXS@
	cd glue/perl; $(MAKE)

if BUILD_HTTPD

install-exec-local: httpd_install

httpd_install:
	cd @APACHE2_SRC@; $(MAKE) install

endif
