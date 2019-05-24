FILES= access.o command.o expr.o format.o input.o main.o output.o pcs.o print.o runpcs.o sym.o regs.o mchpr.o mchframe.o mchsys.o trcrun.o mchinit.o mchio.o misc.o das.o optab.o setup.o syminit.o mchrun.o
CFLAGS=-I. -I../comm

adb: $(FILES)
	$(CC) -o adb $(FILES)

clean:
	rm adb $(FILES)

access.o: ../comm/access.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../comm/access.c
command.o: ../comm/command.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../comm/command.c
expr.o: ../comm/expr.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../comm/expr.c
format.o: ../comm/format.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../comm/format.c
input.o: ../comm/input.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../comm/input.c
main.o: ../comm/main.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../comm/main.c
output.o: ../sun/output.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../sun/output.c
pcs.o: ../comm/pcs.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../comm/pcs.c
print.o: ../comm/print.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../comm/print.c
runpcs.o: ../comm/runpcs.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../comm/runpcs.c
sym.o: ../comm/sym.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../comm/sym.c
regs.o: ../sun/regs.c  ../sun/regs.h ../comm/defs.h
	$(CC) $(CFLAGS) -c ../sun/regs.c
mchrun.o: ../sun/mchrun.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../sun/mchrun.c
mchpr.o: ../sun/mchpr.c 
	$(CC) $(CFLAGS) -c ../sun/mchpr.c
mchframe.o: ../sun/mchframe.c 
	$(CC) $(CFLAGS) -c ../sun/mchframe.c
mchsys.o: ../null/mchsys.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c -I../null ../null/mchsys.c
trcrun.o: ../v8/trcrun.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c -I../v8 ../v8/trcrun.c
mchinit.o: ../null/mchinit.c 
	$(CC) $(CFLAGS) -c -I../null ../null/mchinit.c
mchio.o: ../sun/mchio.c 
	$(CC) $(CFLAGS) -c ../sun/mchio.c
das.o: ../sun/das.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../sun/das.c
optab.o: ../sun/optab.c ../sun/noptab
	$(CC) $(CFLAGS) -c ../sun/optab.c
setup.o: ../sun/setup.c ../comm/defs.h ../sun/machine.h ../sun/base.h
	$(CC) $(CFLAGS) -c ../sun/setup.c
syminit.o: ../v8/syminit.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../v8/syminit.c
