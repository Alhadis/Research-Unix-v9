CC=	newcc
CFLAGS=	-O
FILES=	ar cat chmod cmp cp date dd df du echo ed \
	fsck getty grep init kill ln login ls mkdir \
	mkfs mount mv newgrp nice pr printenv procmount \
	pwd ranlib rm rmdir size stty sync tar test \
	time umount update who write

all: $(FILES)

install: $(FILES)
	/bin/mv $(FILES) /n/labsun/src/sys/raw/test

clean:
	rm -f *.o $(FILES)

ar: ar.o
	$(CC) -o $@ $@.o

cat: cat.o
	$(CC) -o $@ $@.o

chmod: chmod.o
	$(CC) -o $@ $@.o

cmp: cmp.o
	$(CC) -o $@ $@.o

cp: cp.o
	$(CC) -o $@ $@.o

date: date.o
	$(CC) -o $@ $@.o

dd: dd.o
	$(CC) -o $@ $@.o

df: df.o
	$(CC) -o $@ $@.o

du: du.o
	$(CC) -o $@ $@.o

echo: echo.o
	$(CC) -o $@ $@.o

ed: ed.o
	$(CC) -o $@ $@.o

fsck: fsck.o
	$(CC) -o $@ $@.o

getty: getty.o
	$(CC) -o $@ $@.o

grep: grep.o
	$(CC) -o $@ $@.o

init: init.o
	$(CC) -o $@ $@.o

kill: kill.o
	$(CC) -o $@ $@.o

ln: ln.o
	$(CC) -o $@ $@.o

login: login.o
	$(CC) -o $@ $@.o

ls: ls.o
	$(CC) -o $@ $@.o

mkdir: mkdir.o
	$(CC) -o $@ $@.o

mkfs: mkfs.o
	$(CC) -o $@ $@.o

mount: mount.o
	$(CC) -o $@ $@.o

mv: mv.o
	$(CC) -o $@ $@.o

newgrp: newgrp.o
	$(CC) -o $@ $@.o

nice: nice.o
	$(CC) -o $@ $@.o

pr: pr.o
	$(CC) -o $@ $@.o

printenv: printenv.o
	$(CC) -o $@ $@.o

procmount: procmount.o
	$(CC) -o $@ $@.o

pwd: pwd.o
	$(CC) -o $@ $@.o

ranlib: ranlib.o
	$(CC) -o $@ $@.o

rm: rm.o
	$(CC) -o $@ $@.o

rmdir: rmdir.o
	$(CC) -o $@ $@.o

size: size.o
	$(CC) -o $@ $@.o

stty: stty.o
	$(CC) -o $@ $@.o

sync: sync.o
	$(CC) -o $@ $@.o

tar: tar.o
	$(CC) -o $@ $@.o

test: test.o
	$(CC) -o $@ $@.o

time: time.o
	$(CC) -o $@ $@.o

umount: umount.o
	$(CC) -o $@ $@.o

update: update.o
	$(CC) -o $@ $@.o

who: who.o
	$(CC) -o $@ $@.o

write: write.o
	$(CC) -o $@ $@.o
