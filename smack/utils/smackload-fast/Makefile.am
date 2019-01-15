instdir = ${bindir}
bin_PROGRAMS = smackload-fast
noinst_PROGRAMS = genmodes

genmodes_SOURCES = genmodes.c
smackload_fast_SOURCES = main.c input.c output.c common.h
nodist_smackload_fast_SOURCES = modes.h

output.$(OBJEXT): modes.h
CLEANFILES = modes.h

modes.h: genmodes
	./genmodes > $@
