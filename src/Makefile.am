AM_CPPFLAGS = @VMOD_INCLUDES@ -Wall -Werror

vmoddir = @VMOD_DIR@
vmod_LTLIBRARIES = libvmod_bodyaccess.la

libvmod_bodyaccess_la_LDFLAGS = -module -export-dynamic -avoid-version -shared

libvmod_bodyaccess_la_SOURCES = \
	vcc_if.c \
	vcc_if.h \
	vmod_bodyaccess.c \
	vmod_core.c  

vcc_if.c vcc_if.h: @VMODTOOL@ $(top_srcdir)/src/vmod_bodyaccess.vcc
	@VMODTOOL@ $(top_srcdir)/src/vmod_bodyaccess.vcc

VMOD_TESTS = $(top_srcdir)/src/tests/*.vtc
.PHONY: $(VMOD_TESTS)

$(top_srcdir)/src/tests/*.vtc:
	@VARNISHTEST@ -Dvarnishd=@VARNISHD@ -Dvmod_topbuild=$(abs_top_builddir) $@

check: $(VMOD_TESTS)

EXTRA_DIST = \
	vmod_bodyaccess.vcc \
	$(VMOD_TESTS)

CLEANFILES = $(builddir)/vcc_if.c $(builddir)/vcc_if.h
