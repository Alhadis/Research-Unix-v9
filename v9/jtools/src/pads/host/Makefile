OFILES=cache.o pad.o term.o menu.o remote.o sf.o padrcv.o stats.o help.o\
	libC.o trace.o popen.o extern.o
WD=/usr/jtools/src/pads/host
cppC=/usr/lib/cpp
cfrontC=/usr/shopiro/C++/SFcfront/cfront

libpads.a: $(OFILES)
	ar cr libpads.a $(OFILES)
	ranlib libpads.a

trace.o: trace.c
	cc -c -g $(WD)/trace.c

.c.o:	../pads.pri
	CC  -I.. -I/usr/include -c -g -DBSD $(WD)/$*.c

install: libpads.a
	cp libpads.a /usr/jerq/lib
	ranlib /usr/jerq/lib/libpads.a

clean:
	rm -f *.i *junk* core jim.recover

a.out:	test.o libpads.a
	CC test.o libpads.a
