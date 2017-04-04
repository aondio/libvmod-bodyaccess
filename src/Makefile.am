AM_CPPFLAGS = @VMOD_INCLUDES@ -Wall -Werror

TESTS_ENVIRONMENT = PATH=$(LIBVARNISHAPI_BINDIR):$(LIBVARNISHAPI_SBINDIR):$$PATH

vmoddir = @VMOD_DIR@
vmod_LTLIBRARIES = libvmod_bodyaccess.la

libvmod_bodyaccess_la_LDFLAGS = -module -export-dynamic -avoid-version -shared

libvmod_bodyaccess_la_SOURCES = \
	vmod_bodyaccess.c \
	vmod_core.c

nodist_libvmod_bodyaccess_la_SOURCES = \
	vcc_if.c \
	vcc_if.h

dist_man_MANS = vmod_bodyaccess.3

vmod_bodyaccess.3: vmod_bodyaccess.man.rst
	${RST2MAN} $< $@

vmod_bodyaccess.lo: vcc_if.c

vmod_bodyaccess.man.rst vcc_if.c: vcc_if.h

vcc_if.h: @VMODTOOL@ $(top_srcdir)/src/vmod_bodyaccess.vcc
	@PYTHON@ @VMODTOOL@ $(top_srcdir)/src/vmod_bodyaccess.vcc

VTC_LOG_COMPILER = $(VARNISHTEST) -v \
	-Dvmod_topbuild=$(abs_top_builddir)
TEST_EXTENSIONS = .vtc
TESTS = @VMOD_TESTS@

@VMOD_TESTS@: vmod_bodyaccess.lo

EXTRA_DIST = \
	vmod_bodyaccess.vcc \
	$(VMOD_TESTS)

CLEANFILES = \
	$(builddir)/vcc_if.c \
	$(builddir)/vcc_if.h \
	$(builddir)/vmod_bodyaccess.rst \
	$(builddir)/vmod_bodyaccess.man.rst \
	$(dist_man_MANS)
