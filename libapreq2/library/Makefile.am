EXTRA_DIST = t
AM_CPPFLAGS = @APR_INCLUDES@
BUILT_SOURCES = @APR_LA@ @APU_LA@
lib_LTLIBRARIES = libapreq2.la
libapreq2_la_SOURCES = util.c version.c cookie.c param.c parser.c \
                       parser_urlencoded.c parser_header.c parser_multipart.c \
	               module.c module_custom.c module_cgi.c error.c
libapreq2_la_LDFLAGS = -version-info @APREQ_LIBTOOL_VERSION@ @APR_LTFLAGS@ @APR_LIBS@

test: all
	cd t; $(MAKE) test

if ENABLE_PROFILE

AM_CFLAGS= -pg -fprofile-arcs -ftest-coverage

clean-local:
	-rm *.bb *.bbg *.da *.gcov

else

clean-local:

endif



if BUILD_APR

@APR_LA@:
	cd `@APR_CONFIG@ --srcdir` && $(MAKE)

endif

if BUILD_APU

@APU_LA@: @APR_LA@
	cd `@APU_CONFIG@ --srcdir` && $(MAKE)

endif
