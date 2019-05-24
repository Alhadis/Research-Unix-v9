#
# @(#)Makefile 1.1 86/02/03 SMI
#

CSRC = main.c alloc.c branch.c error.c hardexp.c inst.c instruction.c \
       label.c livereg.c pseudoop.c register.c regmask.c scan.c sym.c \
       util.c operand.c quicken.c stackops.c coalesce.c
HSRC = as.h c2.h cctypes.h  inst.h Instructions oper.h
HLIST = as.h c2.h inst.h oper.h
OBJ =	error.o inst.o instruction.o label.o main.o scan.o sym.o util.o \
	branch.o hardexp.o livereg.o regmask.o register.o pseudoop.o \
	alloc.o operand.o quicken.o stackops.o coalesce.o
CC=newcc
COPT=-O
DEFS= -DC2
POPT= -P1 -p
LDFLAGS=
CFLAGS = $(DEFS) $(COPT)
DESTDIR=
c2: $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o c2
inst.o: characteristics inst.c as.h c2.h  inst.h
	$(CC) $(CFLAGS) -c inst.c
characteristics: Instructions awkprog
	(echo FLAVOR C2; cat Instructions) | awk -f awkprog > characteristics
clean:
	rm -rf $(OBJ) core errs c2 lintout characteristics
install: c2
	install -s c2 $(DESTDIR)/lib/c2
print: $(CSRC)
	lpr $(POPT) Makefile $(HSRC)
	lpr $(POPT) $(CSRC)
howbig: $(CSRC) $(HSRC)
	@wc $(CSRC) $(HSRC)
lint:
	lint $(DEFS) $(CSRC) > lintout
main.o: main.c $(HLIST) cctypes.h
alloc.o: alloc.c $(HLIST)
branch.o: branch.c $(HLIST)
error.o:  error.c  $(HLIST)
hardexp.o:as.h $(HLIST)
instruction.o: instruction.c $(HLIST)
label.o: label.c $(HLIST)
livereg.o: livereg.c $(HLIST)
pseudoop.o: pseudoop.c  $(HLIST)
quicken.o: quicken.c $(HLIST)
register.o: register.c $(HLIST)
regmask.o: regmask.c $(HLIST)
scan.o: scan.c $(HLIST)
sym.o: sym.c $(HLIST)
util.o: util.c $(HLIST)
stackops.o: stackops.c $(HLIST)
coalesce.o: coalesce.c $(HLIST)
