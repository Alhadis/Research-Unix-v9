CFLAGS=-g -I/usr/include/sys
OBJ=file.o input.o main.o output.o perm.o work.o
SRC=share.h file.c input.c main.c output.c perm.c work.c setup.c
zarf:	$(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o zarf
all:	setup zarf backup unmount
setup:	setup.o
	$(CC) $(CFLAGS) setup.o -lipc -o setup
$(OBJ):	share.h
unmount:	unmount.o funmount.o
	$(CC) $(CFLAGS) unmount.o funmount.o -o unmount
list:	$(SRC) /usr/sys/sys/netb.c /usr/sys/h/netb.h
	prcan $? && touch list
backup:	cray shamash purdy
cray:	$(SRC) makefile
	push 3k $? share && touch cray
shamash:	$(SRC) makefile
	/usr/ipc/bin/rcp $? shamash:share && touch shamash
purdy:	$(SRC) makefile
	/usr/ipc/bin/rcp $? purdy:share && touch purdy
