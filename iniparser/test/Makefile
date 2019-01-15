#
# iniparser tests Makefile
#

CC      = gcc
CFLAGS  = -g -I../src
LFLAGS  = -L.. -liniparser
AR	    = ar
ARFLAGS = rcv
RM      = rm -f


default: all

all: iniexample parse iniexample_thread

iniexample: iniexample.c
	$(CC) $(CFLAGS) -o iniexample iniexample.c -I../src -L.. -liniparser

parse: parse.c
	$(CC) $(CFLAGS) -o parse parse.c -I../src -L.. -liniparser

iniexample_thread: iniexample_thread.c
	$(CC) $(CFLAGS) -o iniexample_thread iniexample_thread.c -I../src -L.. -liniparser -lpthread

clean veryclean:
	$(RM) iniexample example.ini parse iniexample_thread example_*.ini



