# Makefile for EX291 DOS driver and test program
#  By Peter Johnson, 2000
#
# $Id: Makefile,v 1.2 2000/12/18 06:28:00 pete Exp $

ifndef PMODELIB
all:
	@echo Please set PMODELIB to point to the ECE291 Protected Mode Library.
	@echo For example:
	@echo  SET PMODELIB=v:/pmodelib
else
TARGETDIR = ../../bin

TARGETS = $(TARGETDIR)/ex291.com $(TARGETDIR)/ex291w.com $(TARGETDIR)/test.exe

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

$(TARGETDIR)/ex291.com: extra291.asm
	nasm -f bin $< -o $@

$(TARGETDIR)/ex291w.com: extra291.asm
	nasm -f bin -DWINDOWEDMODE $< -o $@

%.o: %.asm
	nasm -f coff -i$(PMODELIB)/include/ $<

$(TARGETDIR)/%.exe: %.o
	gcc -o $@ $< $(PMODELIB)/lib291.a
endif

