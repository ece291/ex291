# Makefile for EX291 VBE/AF driver
#  By Peter Johnson, 2000
#
# Based on FreeBE/AF makefile.  The freebe/ subdirectory contains the FreeBE/AF
#  build tools and helper functions.
#
# $Id: Makefile,v 1.1 2001/01/08 07:07:39 pete Exp $

TARGETDIR = ../../bin

TARGETS = $(TARGETDIR)/ex291.drv

ifdef PGCC
CFLAGS = -O6 -mpentium -fomit-frame-pointer -Wall -Werror
else
CFLAGS = -O3 -m486 -fomit-frame-pointer -Wall -Werror
endif

IFLAGS = -Ifreebe

all: $(TARGETS)

$(TARGETDIR)/ex291.drv: freebe/drvgen.exe freebe/start.o freebe/helper.o $(subst drvhdr.o ,,$(subst .c,.o,$(wildcard *.c)))
	freebe/drvgen $@ OemExt PlugAndPlayInit StartDriver $(subst freebe/drvgen.exe ,,$^)

freebe/drvgen.exe: freebe/drvgen.o drvhdr.o
	gcc -s -o $@ $^

%.o: %.c
	gcc $(CFLAGS) $(IFLAGS) -o $@ -c $<

%.o: %.s
	gcc -x assembler-with-cpp -o $@ -c $<

clean:
	rm -f *.o freebe/drvgen.exe $(TARGETS)

veryclean: clean
	rm -f freebe/*.o

