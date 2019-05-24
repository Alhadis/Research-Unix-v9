#
# @(#)Makefile	1.1	86/02/03	SMI
#
CC	= cc
MODE = -O
#MODE = -O -pg
#MODE = -g
CFLAGS = -DAS $(MODE) -fsoft -DEBUG
LDFLAGS = $(MODE)
ASH = as.h inst.h error.h oper.h
KRUD = inst.o instruction.o op.o ps.o stab.o scan.o rel.o sym.o sdi.o init.o error.o operand.o eaddr.o coprocessor.o
as: $(KRUD)
	$(CC) $(LDFLAGS) $(KRUD) -o as
install: as
	install -s as $(DESTDIR)/bin/as
instruction.o: instruction.c $(ASH)
op.o: op.c $(ASH)
ps.o: ps.c $(ASH)
stab.o: stab.c scan.h $(ASH)
scan.o: scan.c $(ASH) scan.h
rel.o: rel.c $(ASH)
sym.o: sym.c scan.h $(ASH)
sdi.o: sdi.c $(ASH)
init.o: init.c $(ASH)
eaddr.o: eaddr.c $(ASH)
coprocessor.o: coprocessor.c $(ASH)
operand.o: operand.c $(ASH)
	$(CC) $(CFLAGS) -R -c operand.c
as_charac:	Instructions awkprog
	awk -f awkprog < Instructions >as_charac
inst.o: inst.c as_charac $(ASH)
	$(CC) $(CFLAGS) -R -c inst.c
#	$(CC) $(CFLAGS) -c inst.c
error.o: error.c $(ASH)
	$(CC) $(CFLAGS) -R -c error.c
clean:;	rm -f $(KRUD) errs as_charac
