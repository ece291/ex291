# Makefile for EX291 DOS driver and test program
#  By Peter Johnson, 2000
#
# $Id: Makefile,v 1.3 2001/01/09 22:36:26 pete Exp $

ifndef PMODELIB
all:
	@echo Please set PMODELIB to point to the ECE291 Protected Mode Library.
	@echo For example:
	@echo  SET PMODELIB=v:/pmodelib
else
TARGETDIR = ../../bin

TARGETS = $(TARGETDIR)/ex291.com $(TARGETDIR)/test.exe

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

$(TARGETDIR)/ex291.com: extra291.asm
	nasm -f bin $< -o $@

%.o: %.asm
	nasm -f coff -i$(PMODELIB)/include/ $<

$(TARGETDIR)/%.exe: %.o
	gcc -o $@ $< $(PMODELIB)/lib291.a
endif

