# @(#) $Header: Makefile,v 1.1 87/08/26 00:23:00 leres Exp $ (LBL)
#
# Sun vfont to portable bdf font converter.
#

CFLAGS= -O
PROG= vtobdf
PROG= vtobdf

.SUFFIXES: .sun .bdf .snf

.sun.bdf:
	./vtobdf $*.sun $*.bdf

.bdf.snf:
	./fc $*.bdf >$*.snf

all: vtobdf

vtobdf: vtobdf.c
	${CC} ${CFLAGS} vtobdf.c -o vtobdf

clean:
	rm -f vtobdf *.o *.bdf *.snf

lint:
	lint -hbxn vtobdf.c

show: serif-11.snf
	./showsnf serif-11.snf

shar:
	shar -v -c -p X README Makefile vtobdf.c
