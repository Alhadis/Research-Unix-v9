
CFLAGS= -I../../../include -g

mkfs: mkfs.o ltol3.o
	cc ${CFLAGS} -o mkfs mkfs.o ltol3.o

fsck: fsck.o ltol3.o
	cc ${CFLAGS} -o fsck fsck.o ltol3.o

printfs: printfs.o
	cc ${CFLAGS} -o printfs printfs.o

wrfs: wrfs.o
	cc ${CFLAGS} -o wrfs wrfs.o

rdfs: rdfs.o
	cc ${CFLAGS} -o rdfs rdfs.o

v9tar: v9tar.c
	cc -o v9tar v9tar.c

clean:
	rm -f *.o rdfs wrfs printfs fsck mkfs v9tar
