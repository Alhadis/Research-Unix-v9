#
# @(#)Makefile.sun3 1.1 86/02/03 SMI
#

DESTDIR=/
CFLAGS=	-O $(COPTS)
COPTS= -DSUN3

# BRELOC is text segment start for final stage (2nd or 3rd stage) boots
# Note that this must be higher than the kernel's edata
BRELOC= 90000
# LOAD is text segment start for booted user programs
LOAD=	4000

DRIVERS=tm.o nd.o xy.o sd.o st.o sc.o si.o xt.o if_ie.o \
	if_le.o getidprom.o idprom.o common.o inet.o

LIBSA=	$(DRIVERS) sys.o devio.o conf.o standalloc.o s3map.o \
	prf.o get.o ldivt.o lmodt.o probe.o chklabel.o spinning.o stubs.o

RM=	rm -f

ALL=	libsa.a srt0.o boot tpboot copy tpcopy \
	bootpr bootxy bootsd installboot.sh \
	ndboot.sun3.private ndboot.sun3.pub0 ndboot.sun3.pub1

all: ${ALL}

sun2:	FRC
	rm Makefile
	ln -s Makefile.sun2 Makefile
	make clean;make

sun3:	FRC
	rm Makefile
	ln -s Makefile.sun3 Makefile
	make clean;make

FRC:

s3map.o: s3map.s
	rm -f tmp.c s3map.i s3map.o
	cp s3map.s tmp.c
	${CC} -E -DASM ${CFLAGS} tmp.c > s3map.i
	${AS} ${LFLAG} s3map.i -o s3map.o
	rm -f tmp.c s3map.i

getidprom.o: getidprom.s
	rm -f tmp.c getidprom.i getidprom.o
	cp getidprom.s tmp.c
	${CC} -E -DASM ${CFLAGS} tmp.c >getidprom.i
	${AS} ${LFLAG} getidprom.i -o getidprom.o
	rm -f tmp.c getidprom.i

libsa.a: $(LIBSA)
	ar crv libsa.a $?
	ranlib libsa.a

srt0.o: srt0.s
	rm -f tmp.c srt0.i srt0.o
	cp srt0.s tmp.c
	cc -E ${CFLAGS} tmp.c > srt0.i
	${AS} srt0.i -o srt0.o
	rm -f tmp.c srt0.i

readfile.o: readfile.c
	cc -c ${CFLAGS} -DLOAD=0x${LOAD} readfile.c

# Standalone, generic boot program -- get it from anywhere, it will
# boot anything.  "tpboot" has the a.out header stripped off so you
# can write it on the first file of a tape.
boot:	boot.o readfile.o srt0.o libsa.a
	ld -N -T ${BRELOC} -o boot srt0.o boot.o readfile.o libsa.a -lc

sboot:	sboot.o readfile.o srt0.o libsa.a
	ld -N -T ${BRELOC} -o sboot srt0.o sboot.o readfile.o libsa.a -lc

tpboot:	tpboot.o readfile.o srt0.o libsa.a
	ld -N -T ${BRELOC} srt0.o tpboot.o readfile.o libsa.a -lc
	cp a.out b.out; strip b.out; dd if=b.out of=tpboot ibs=32 skip=1; ${RM} b.out

tpboot.o: boot.c
	cp boot.c tpboot.c; chmod +w tpboot.c
	cc -c ${CFLAGS} -DJUSTASK tpboot.c
	${RM} tpboot.c

sboot.o: boot.c
	cp boot.c sboot.c; chmod +w sboot.c
	cc -c ${CFLAGS} -DJUSTASK sboot.c
	${RM} sboot.c

# Standalone copy program for copying eg, tape->disk
copy:	copy.o srt0.o libsa.a
	ld -N -T ${LOAD} -o copy srt0.o copy.o libsa.a -lc

# bootable from tape
tpcopy:	copy.o srt0.o libsa.a
	ld -T ${LOAD} -o tpcopy srt0.o copy.o libsa.a -lc

# sys version for 2nd stage which uses RAM driver and needs 3rd stage
sysxx.o: sys.c
	${RM} sysxx.c sysxx.o
	cp sys.c sysxx.c
	${CC} -c ${CFLAGS} -DBOOTBLOCK sysxx.c
	${RM} sysxx.c

# getting booted from anywhere (generic = pr for Prom)

bootpr: srt0.o bootxx.o readfile.o confpr.o sysxx.o libsa.a sizecheck
	ld -N -T ${BRELOC} srt0.o bootxx.o readfile.o confpr.o sysxx.o \
		libsa.a -lc
	sizecheck
	cp a.out b.out;strip b.out;dd if=b.out of=bootpr ibs=32 skip=1


bootxy: srt0.o bootxx.o readfile.o confxy.o sysxx.o xy.boot.o libsa.a sizecheck
	ld -N -T ${BRELOC} srt0.o bootxx.o readfile.o confxy.o sysxx.o \
		xy.boot.o libsa.a -lc
	sizecheck
	cp a.out b.out;strip b.out;dd if=b.out of=bootxy ibs=32 skip=1

xy.boot.o: xy.c
	rm -f tmp.c tmp.o xy.boot.o
	cp xy.c tmp.c
	cc -c ${CFLAGS} -DBOOTBLOCK tmp.c
	mv tmp.o xy.boot.o

confxy.o: confxx.c
	cc -c ${CFLAGS} -Dxxdriver=xydriver confxx.c
	mv confxx.o confxy.o

bootsd: bootpr
	cp bootpr bootsd

big.bootsd: srt0.o bootxx.o readfile.o confsd.o sysxx.o sd.boot.o libsa.a sizecheck
	ld -N -T ${BRELOC} srt0.o bootxx.o readfile.o confsd.o sd.boot.o \
		sysxx.o libsa.a -lc
	sizecheck
	cp a.out b.out;strip b.out;dd if=b.out of=bootsd ibs=32 skip=1

sd.boot.o: sd.c
	rm -f tmp.c tmp.o sd.boot.o
	cp sd.c tmp.c
	cc -c ${CFLAGS} -DBOOTBLOCK tmp.c
	mv tmp.o sd.boot.o

confsd.o: confxx.c
	cc -c ${CFLAGS} -Dxxdriver=sddriver confxx.c
	mv confxx.o confsd.o

sizecheck: SCCS/s.sizecheck
	sccs get sizecheck
	chmod +x sizecheck

# various flavors of tftp-bootable ND booters

ndboot.sun3.private: boot
	cp boot b.out
	echo 'ndbootdev?w 0' | adb -w b.out
	strip b.out; dd if=b.out of=$@ ibs=32 skip=1; ${RM} b.out

ndboot.sun3.pub0: boot
	cp boot b.out
	echo 'ndbootdev?w 40' | adb -w b.out
	strip b.out; dd if=b.out of=$@ ibs=32 skip=1; ${RM} b.out

ndboot.sun3.pub1: boot
	cp boot b.out
	echo 'ndbootdev?w 41' | adb -w b.out
	strip b.out; dd if=b.out of=$@ ibs=32 skip=1; ${RM} b.out

# misc
installboot.sh:
	sccs get installboot.sh


# Test program: sacat (standalone cat)

sacat: sacat.o srt0.o libsa.a
	ld -N -T ${BRELOC} -o sacat srt0.o sacat.o libsa.a -lc

# utilities

depend:
	@echo 'making dependencies ...'
	@grep '^#include' *.c saio.h | grep -v '<' | \
	      sed 's/:[^"]*"\([^"]*\)".*/: \1/' | \
	      sed 's/\.[cs]/.o/' >depend.tmp
	@awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		       else rec = rec " " $$2 } } \
	      END { print rec } ' <depend.tmp  > makedep
	@echo '/^# DO NOT DELETE THIS LINE/+1,$$d' >eddep
	@echo '$$r makedep' >>eddep
	@echo 'w' >>eddep
	@chmod +w Makefile
	@ed - Makefile < eddep
	@rm eddep makedep depend.tmp
	@echo '... done'

clean:
	rm -f ${ALL}
	rm -f *.o *.exe *.i *.d core errs 
	rm -f a.out b.out sacat
	rm -f boot[a-z]? sysxx.c syspr.c tmp.c

lint:
	lint ${COPTS} -hxbn *.c | \
	    grep -v 'possible pointer alignment' | \
	    grep -v 'struct/union .* never defined'

install: ${ALL}
	-mkdir ${DESTDIR}/stand ${DESTDIR}/usr/mdec && \
		chown bin ${DESTDIR}/stand ${DESTDIR}/usr/mdec && \
		chmod 755 ${DESTDIR}/stand ${DESTDIR}/usr/mdec
	install boot ${DESTDIR}
	install copy ${DESTDIR}/stand
	install bootxy ${DESTDIR}/usr/mdec/bootxy
	install bootsd ${DESTDIR}/usr/mdec/bootsd
	install bootpr ${DESTDIR}/usr/mdec/bootpr
	install bootpr ${DESTDIR}/usr/mdec/bootnd
	install -c -m 755 installboot.sh ${DESTDIR}/usr/mdec/installboot
	-mkdir ${DESTDIR}/tftpboot
	install ndboot.sun3.private ${DESTDIR}/tftpboot
	install ndboot.sun3.pub0 ${DESTDIR}/tftpboot
	install ndboot.sun3.pub1 ${DESTDIR}/tftpboot


# DO NOT DELETE THIS LINE -- make depend uses it

