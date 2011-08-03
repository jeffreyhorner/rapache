# DARWIN has to be different than everyone else
# and define their autoconf $shlibpath_var to be DYLD_LIBRARY_PATH
SHLIBPATH_VAR=@SHLIBPATH_VAR@

ACLOCAL_AMFLAGS = -I m4

APXS=@APXS@
HTTPD=@HTTPD@
DOCROOT=@DOCROOT@

APREQ=@APREQ@
APREQ_COMPILE=@APREQ_COMPILE@
APREQ_INSTALL=@APREQ_INSTALL@
APREQ_CLEAN=@APREQ_CLEAN@
APREQ_LIB_DIR=@APREQ_LIB_DIR@
APREQ_INCLUDES=@APREQ_INCLUDES@
APREQ_LINKLD=@APREQ_LINKLD@

RPROG=@RPROG@
RPROGBATCH=R_DEFAULT_PACKAGES=NULL @RPROG@ --vanilla --no-readline --slave -f
R_HOME = `$(RPROG) RHOME`
R_LD_LIBRARY_PATH = $(R_HOME)/lib
RINCLUDES=@RINCLUDES@
RLINKLD=@RLINKLD@

INCLUDES = $(RINCLUDES) $(APREQ_INCLUDES)
LDFLAGS = $(RLINKLD) $(APREQ_LINKLD)
LD_LIBRARY_PATH=$(R_LD_LIBRARY_PATH):$(APREQ_LIB_DIR)
RPATH=@RPATH@

all: mod_R.so $(DOCROOT)/httpd.conf

mod_R.so: .apreq-config mod_R.c mod_R.h
	@echo
	@echo Compiling mod_R
	@echo
	$(APXS) $(INCLUDES) -c mod_R.c $(RPATH) $(LDFLAGS)

.apreq-config:
	@echo
	@echo Configuring libapreq2
	@echo
	$(APREQ_COMPILE)
	touch .apreq-config

install: all
	$(APXS) -i -n R mod_R.la
	$(APREQ_INSTALL)

$(DOCROOT)/httpd.conf: $(DOCROOT)/httpd.conf.in
	$(RPROGBATCH) tools/config_http.R --args $(APXS) $(HTTPD)

itest: all
	@echo
	@echo Point your browser to http://localhost:8181/index.html
	@echo
	$(SHLIBPATH_VAR)=$(LD_LIBRARY_PATH) R_HOME=$(R_HOME) $(HTTPD) -X -f $(DOCROOT)/httpd.conf

test: all $(DOCROOT)/httpd.conf

stop: all
	$(SHLIBPATH_VAR)=$(LD_LIBRARY_PATH) R_HOME=$(R_HOME) $(HTTPD) -X -f $(DOCROOT)/httpd.conf -k stop

valgrind: all
	$(SHLIBPATH_VAR)=$(LD_LIBRARY_PATH) R_HOME=$(R_HOME) valgrind $(HTTPD) -X -f $(DOCROOT)/httpd.conf

debug: all
	@echo
	@echo Copy/paste the following line to gdb
	@echo
	@echo run -X -f $(DOCROOT)/httpd.conf 
	@echo
	$(SHLIBPATH_VAR)=$(LD_LIBRARY_PATH) R_HOME=$(R_HOME) gdb $(HTTPD)

clean:
	rm -rf $(OBJS) core mod_R.o mod_R.so *~ .libs *.o *.slo *.lo *.la .apreq-config
	$(APREQ_CLEAN)

distclean: clean
	rm -rf mod_R.h Makefile .depend .install libtool config.log config.status test/httpd.conf test/access_log test/error_log test/httpd.pid test/accept.lock* aclocal.m4 autom4te.cache libapreq2/library/t/Makefile libapreq2/config.nice libapreq2/module/apache/Makefile
	$(APREQ_CLEAN)
