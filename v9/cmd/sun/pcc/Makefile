#
# @(#)Makefile 1.1 86/02/03 Sun Micro 1984
#
CC	= cc
TESTDIR = .
DESTDIR =
LDFLAGS = 
COPTS =  -O
INSTALL= install -s -m 555
CFLAGS = -DIEEECCODES -DBUFSTDERR $(COPTS)
FFLAGS = $(CFLAGS)  # for Fortran only
M=../mip
CGRDR = ../cgrdr
CSRC =	$M/allo.c $M/cgram.y $M/comm1.c $M/mip.h $M/match.c \
		$M/cpass1.h $M/cpass2.h $M/optim.c $M/pftn.c $M/reader.c \
		$M/scan.c $M/trees.c $M/xdefs.c $M/yyerror.c \
		$M/common \
		regvars.c \
		code.c  \
		bound.c local.c  \
		local2.c  opmatch.c optim2.c \
		order.c su.c \
		stab.c float2.c \
		flags2.c \
		table.c  \
		util2.c \
		machdep2.h machdep.h 
FSRC =	$M/fallo.c $M/fcomm2.c $M/flocal2.c $M/fmatch.c $M/forder.c \
		$M/freader.c $M/ftable.c fbound.c ffloat2.c \
		fflags2.c foptim2.c fopmatch.c futil2.c fsu.c
COBJ = rodata.o cgram.o xdefs.o scan.o pftn.o trees.o optim.o code.o local.o \
	regvars.o reader.o local2.o order.o opmatch.o match.o allo.o comm1.o \
	bound.o table.o float2.o flags2.o stab.o optim2.o util2.o su.o yyerror.o
FOBJ = freader.o fallo.o fopmatch.o fmatch.o ftable.o forder.o \
	fbound.o flocal2.o fcomm2.o ffloat2.o fflags2.o foptim2.o futil2.o fsu.o
RDIR_OBJ = fort.o $(CGRDR)/cgrdr.o

all: comp fort cg

comp: $(COBJ)
	$(CC) $(LDFLAGS)  $(COBJ)
	mv a.out $(TESTDIR)/comp
trees.o: $M/mip.h machdep.h $M/cpass1.h $M/trees.c
	$(CC) -c $(CFLAGS) -I$M -I.  $M/trees.c
optim.o: $M/mip.h machdep.h $M/cpass1.h $M/optim.c
	$(CC) -c $(CFLAGS) -I$M -I. $M/optim.c
pftn.o: $M/mip.h machdep.h $M/cpass1.h $M/pftn.c
	$(CC) -c $(CFLAGS) -I$M -I. $M/pftn.c
regvars.o: $M/mip.h machdep.h $M/cpass1.h
	$(CC) -c $(CFLAGS) -I$M -I. regvars.c
code.o: $M/mip.h machdep.h $M/cpass1.h
	$(CC) -c $(CFLAGS) -I$M -I. code.c
local.o: $M/mip.h machdep.h $M/cpass1.h
	$(CC) -c $(CFLAGS) -I$M -I. local.c
stab.o: $M/mip.h machdep.h $M/cpass1.h stab.c
	$(CC) -c $(CFLAGS) -I$M -I. stab.c
scan.o: $M/mip.h machdep.h $M/cpass1.h $M/scan.c
	$(CC) -c $(CFLAGS) -I$M -I. $M/scan.c
xdefs.o: $M/mip.h $M/cpass1.h machdep.h $M/xdefs.c
	$(CC) -c $(CFLAGS) -I$M -I. $M/xdefs.c
yyerror.o: $M/mip.h machdep.h $M/cpass1.h $M/yyerror.c
	$(CC) -c $(CFLAGS) -I$M -I. $M/yyerror.c
cgram.o: $M/mip.h $M/cpass1.h machdep.h cgram.c
	$(CC) -c $(CFLAGS) -I$M -I. cgram.c
rodata.o: rodata.c
	$(CC) $(CFLAGS) -c -R rodata.c
rodata.c cgram.c: $M/cgram.y
	$(YACC) $M/cgram.y
	rm -f rodata.c
	./:yyfix yyexca yyact yypact yypgo yyr1 yyr2 yychk yydef
	mv y.tab.c cgram.c
comm1.o: $M/mip.h $M/cpass1.h $M/common machdep.h $M/comm1.c
	$(CC) -c $(CFLAGS) -I$M -I. $M/comm1.c
table.o: $M/mip.h $M/cpass2.h machdep2.h machdep.h table.c
	$(CC) $(CFLAGS) -c -R -I$M -I. table.c
reader.o: $M/mip.h $M/cpass2.h machdep2.h machdep.h $M/reader.c
	$(CC) -c $(CFLAGS) -I$M -I. $M/reader.c
local2.o: $M/mip.h $M/cpass2.h machdep2.h machdep.h
	$(CC) -c $(CFLAGS) -I$M -I. local2.c
bound.o: $M/mip.h $M/cpass2.h machdep2.h machdep.h
	$(CC) -c $(CFLAGS) -I$M -I. bound.c
util2.o: $M/mip.h $M/cpass2.h machdep2.h machdep.h util2.c
	$(CC) -c $(CFLAGS) -I$M -I. util2.c
opmatch.o: $M/mip.h $M/cpass2.h machdep2.h machdep.h 
	$(CC) -c $(CFLAGS) -I$M -I. opmatch.c
optim2.o: $M/mip.h $M/cpass2.h machdep2.h machdep.h 
	$(CC) -c $(CFLAGS) -I$M -I. optim2.c
float2.o: $M/mip.h $M/cpass2.h machdep2.h machdep.h float2.c 
	$(CC) -c $(CFLAGS) -I$M -I. float2.c
flags2.o: $M/mip.h $M/cpass2.h machdep2.h machdep.h flags2.c 
	$(CC) -c $(CFLAGS) -I$M -I. flags2.c
order.o: $M/mip.h $M/cpass2.h machdep2.h machdep.h
	$(CC) -c $(CFLAGS) -I$M -I. order.c
su.o:   $M/mip.h $M/cpass2.h machdep2.h machdep.h su.c
	$(CC) -c $(CFLAGS) -I$M -I. su.c
match.o: $M/mip.h $M/cpass2.h machdep2.h machdep.h $M/match.c
	$(CC) -c $(CFLAGS) -I$M -I. $M/match.c
allo.o: $M/mip.h $M/cpass2.h machdep2.h machdep.h $M/allo.c
	$(CC) -c $(CFLAGS) -I$M -I. $M/allo.c
shrink:
	-rm *.o comp
clean:
	rm -f *.o comp fort cg cgram.c rodata.c
lintall:
	lint -hpv -I. -I$M  $M/cgram.c $M/xdefs.c $M/scan.c $M/pftn.c \
		$M/trees.c $M/optim.c code.c local.c $M/reader \
		local2.c order.c $M/match.c $M/allo.c $M/comm1.c table.c
fort: comp fort.o $(FOBJ)
	$(CC)  $(LDFLAGS) fort.o $(FOBJ) -o $(TESTDIR)/fort
fort.o: fort.h $M/fort.c
	$(CC) -c $(FFLAGS) -I$M -I. $M/fort.c
freader.o: reader.o
	$(CC) -c $(FFLAGS) -I$M -I. $M/freader.c
fallo.o: allo.o
	$(CC) -c $(FFLAGS) -I$M -I. $M/fallo.c
fmatch.o: match.o
	$(CC) -c $(FFLAGS) -I$M -I. $M/fmatch.c
ftable.o: table.o
	$(CC) -c -R $(FFLAGS) -I$M -I. $M/ftable.c
forder.o: order.o
	$(CC) -c $(FFLAGS) -I$M -I. $M/forder.c
fsu.o:    su.o
	$(CC) -c $(FFLAGS) -I$M -I. fsu.c
flocal2.o: local2.o
	$(CC) -c $(FFLAGS) -I$M -I. $M/flocal2.c
fbound.o: bound.o
	$(CC) -c $(FFLAGS) -I$M -I. fbound.c
ffloat2.o: float2.o 
	$(CC) -c $(FFLAGS) -I$M -I. ffloat2.c
fflags2.o: flags2.o 
	$(CC) -c $(FFLAGS) -I$M -I. fflags2.c
foptim2.o: optim2.o 
	$(CC) -c $(FFLAGS) -I$M -I. foptim2.c
fopmatch.o: opmatch.o 
	$(CC) -c $(FFLAGS) -I$M -I. fopmatch.c
futil2.o: util2.o 
	$(CC) -c $(FFLAGS) -I$M -I. futil2.c
fcomm2.o: $M/common
	$(CC) -c $(FFLAGS) -I$M -I. $M/fcomm2.c
cg: comp fort neverthere $(FOBJ)
	$(CC)  $(LDFLAGS) $(CGRDR)/cgrdr.o $(FOBJ) -o $(TESTDIR)/cg
neverthere:; cd $(CGRDR); make ${MFLAGS};
fort.o freader.o fallo.o fmatch.o ftable.o forder.o flocal2.o fcomm2.o: \
	$M/cpass2.h $M/mip.h machdep.h machdep2.h
install:	all
	$(INSTALL) comp $(DESTDIR)/lib/ccom
	$(INSTALL) fort $(DESTDIR)/usr/lib/f1
	$(INSTALL) cg $(DESTDIR)/usr/lib/cg
print:	
	pr -f makefile 
	/usr/ucb/ctags -x ${CSRC} | pr -f -h XREF
	pr -f ${CSRC} $M/fort.c ${FSRC}
tags:
	ctags ${CSRC}
