#CC=	newcc
CFLAGS=	-O

OFILES=	Faddd.o Fadds.o Faintd.o Faints.o Fdtos.o Ffloat.o Ffltd.o \
	Fflts.o Fmuld.o Fmuls.o Fstod.o fp_globals.o frexp.o ldexp.o \
	modf.o is68020.o ffs.o valloc.o

all: $(OFILES)

install:
	mv $(OFILES) ../OBJ

.s.o:
	/lib/cpp $< > $*.i
	as -20 -o $*.o $*.i
	rm $*.i
