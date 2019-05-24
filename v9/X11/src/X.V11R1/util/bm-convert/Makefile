# 
# $Header: Makefile,v 11.0 87/06/24 11:01:26 toddb Exp $ 
# $Locker:  $ 
#
# Copyright 1985, Massachusetts Institute of Technology.
#
# Makefile for xset
#
DESTDIR =
CONFDIR = /usr/new
INCLUDES =
DEPEND=../../makedepend/makedepend
CFLAGS = -O $(INCLUDES)
SRCS = bm-convert.c
OBJS = bm-convert.o
.SUFFIXES: .o .h .c .a

all: bm-convert

install:
	install bm-convert ${DESTDIR}${CONFDIR}

clean: 
	rm -f ${OBJS} bm-convert
	rm -f *.[0-9]* *~* *.bak core \#*

bm-convert: ${OBJS}
	cc $(OBJS) -o $@

lint:	${SRCS}
	lint -I../.. $(SRCS)

depend:
	${DEPEND} -v -I. $(CFLAGS) -s'# DO NOT DELETE' $(SRCS)

# DO NOT DELETE

xset.o: ../../X11/copyright.h ../../X11/X.h ../../X11/Xlib.h
# ../../X11/Xlib.h includes:
#	sys/types.h
#	X11/X.h
xset.o: /usr/include/sys/types.h ../../X11/X.h /usr/include/stdio.h
xset.o: /usr/include/netdb.h /usr/include/netinet/in.h /usr/include/strings.h
xset.o: /usr/include/ctype.h
