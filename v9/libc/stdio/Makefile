CC=	newcc
CFLAGS=	-O

OFILES=	cleanup.o clrerr.o data.o doprnt.o doscan.o fdopen.o fgetc.o \
	fgets.o filbuf.o flsbuf.o fopen.o fprintf.o fputc.o fputs.o \
	freopen.o fseek.o fstab.o ftell.o gcvt.o getchar.o getflds.o \
	getgrent.o getgrgid.o getgrnam.o getpass.o getpwent.o getpwnam.o \
	getpwuid.o gets.o getw.o popen.o printf.o putchar.o puts.o \
	putw.o rdwr.o rew.o scanf.o setbuf.o sprintf.o strout.o \
	stuff.o system.o tmpnam.o ungetc.o

all: $(OFILES)

install:
	mv $(OFILES) ../OBJ

.s.o:
	$(CC) -c $<
