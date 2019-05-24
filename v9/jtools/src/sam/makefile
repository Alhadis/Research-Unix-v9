JDIR=/usr/jtools
LIB=../v9compat/libv9.a

CFILES=sam.c address.c alloc.c buffer.c cmd.c disc.c error.c file.c gcalloc.c \
	io.c list.c mesg.c misc.c moveto.c multi.c rasp.c regexp.c string.c \
	sys.c xec.c unix.c
FILES=sam.o address.o alloc.o buffer.o cmd.o disc.o error.o file.o gcalloc.o \
	io.o list.o mesg.o misc.o moveto.o multi.o rasp.o regexp.o string.o \
	sys.o xec.o unix.o
HFILES=sam.h errors.h parse.h mesg.h
CFLAGS=-g -DSUN -DDIST

sam:	$(FILES) malloc.o sun.o
	rm -f sam
	cc $(CFLAGS) -o sam $(FILES) malloc.o sun.o $(LIB)

sun.o: sam.h errors.h mesg.h

$(FILES) $(OFILES):	sam.h errors.h mesg.h
address.o cmd.o parse.o xec.o unix.o:	parse.h

install: sam
	strip sam
	-rm $(JDIR)/x3bin/sam $(JDIR)/s3bin/sam
	mv sam $(JDIR)/x3bin
	ln $(JDIR)/x3bin/sam $(JDIR)/s3bin

clean:
	rm -f *.o core *.out
