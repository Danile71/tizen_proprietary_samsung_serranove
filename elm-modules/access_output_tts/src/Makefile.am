MAINTAINERCLEANFILES = Makefile.in

pkgdir = $(libdir)/elementary/modules/access_output_tts/$(MODULE_ARCH)
pkg_LTLIBRARIES = module.la

access_output_tts_sounds_dir = ../sounds
include ../sounds/file.list

sounddir = $(pkgdir)/sounds/
sound_DATA = ${access_output_tts_sound_files}

module_la_SOURCES = mod.c

module_la_CPPFLAGS = \
-I. \
-DACCESS_OUTPUT_TTS_SOUND_DIR="\"$(sounddir)\"" \
@ACCESS_OUTPUT_CFLAGS@

module_la_LIBADD = -ltts \
@ACCESS_OUTPUT_LIBS@

module_la_LDFLAGS = -no-undefined -module -avoid-version
module_la_LIBTOOLFLAGS = --tag=disable-static
