## Process this file with automake to produce Makefile.in

bin_PROGRAMS = sndfile-regtest

noinst_HEADERS = regtest.h

INCLUDES = -I$(top_srcdir)/src $(SQLITE3_CFLAGS) $(OS_SPECIFIC_CFLAGS)

COMMON_CFLAGS = -fPIC -pie

sndfile_regtest_SOURCES = sndfile-regtest.c database.c checksum.c
sndfile_regtest_LDADD = $(top_builddir)/src/libsndfile.la $(SQLITE3_LIBS)
sndfile_regtest_CFLAGS = $(COMMON_CFLAGS)

