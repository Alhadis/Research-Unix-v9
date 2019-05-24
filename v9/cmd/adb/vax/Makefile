FILES= access.o command.o expr.o format.o input.o main.o output.o pcs.o print.o runpcs.o sym.o syminit.o trcrun.o mchframe.o mchinit.o mchio.o mchrun.o mchpr.o mchsys.o opset.o optab.o regs.o setup.o
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
output.o: ../comm/output.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../comm/output.c
pcs.o: ../comm/pcs.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../comm/pcs.c
print.o: ../comm/print.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../comm/print.c
runpcs.o: ../comm/runpcs.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../comm/runpcs.c
sym.o: ../comm/sym.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../comm/sym.c
syminit.o: ../v8/syminit.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c -I../v8 ../v8/syminit.c
trcrun.o: ../v8/trcrun.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c -I../v8 ../v8/trcrun.c
mchframe.o: ../vax/mchframe.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../vax/mchframe.c
mchinit.o: ../vax/mchinit.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../vax/mchinit.c
mchio.o: ../vax/mchio.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../vax/mchio.c
mchrun.o: ../vax/mchrun.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../vax/mchrun.c
mchpr.o: ../vax/mchpr.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../vax/mchpr.c
mchsys.o: ../vax/mchsys.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../vax/mchsys.c
opset.o: ../vax/opset.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../vax/opset.c
optab.o: ../vax/optab.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../vax/optab.c
regs.o: ../vax/regs.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../vax/regs.c
setup.o: ../vax/setup.c ../comm/defs.h 
	$(CC) $(CFLAGS) -c ../vax/setup.c
