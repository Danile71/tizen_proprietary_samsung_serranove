MAINTAINERCLEANFILES = aclocal.m4 compile config.sub config.guess config.h.in \
		       configure depcomp install-sh ltmain.sh Makefile.in missing

pkgdir = $(libdir)/edje/modules/feedback/$(MODULE_ARCH)
pkg_LTLIBRARIES = module.la

module_la_SOURCES = feedback.c

module_la_CPPFLAGS = \
@EDJE_FEEDBACK_CFLAGS@

module_la_LIBADD = -leina -ledje -lfeedback \
@EDJE_FEEDBACK_LIBS@

module_la_LDFLAGS = -no-undefined -module -avoid-version
module_la_LIBTOOLFLAGS = --tag=disable-static
