#
# Makefile for fbc utility
#

CC =		gcc -Wall -O2 -I. -lm
BISON =		bison -d
FLEX =		flex
INSTALL =	install
RM =		rm -f

All:		fbc


fbset:		fbc.o

fbset.o:	fbc.c fb.h

install:	fbc
		if [ -f /sbin/fbc ]; then rm /sbin/fbc; fi
		$(INSTALL) fbc /usr/sbin

clean:
		$(RM) *.o fbc
