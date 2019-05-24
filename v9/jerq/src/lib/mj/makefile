L=libmj.a
INCL=/usr/jerq/include
CC=3cc
AS=/usr/jerq/bin/vax/m32as
AR=/usr/jerq/bin/3ar
DCAN=dcan
CFLAGS=

OBJ=arc.o circle.o comm.o cosdeg.o data.o disc.o discture.o elarc.o \
	eldisc.o ellipse.o exit.o fgets.o font.o fopen.o get.o getfont.o \
	getrect.o hmenuhit.o popen.o printf.o put.o rdwr.o

CFILES=arc.c circle.c comm.c cosdeg.c data.c disc.c discture.c elarc.c \
	eldisc.c ellipse.c exit.c fgets.c font.c fopen.c get.c getfont.c \
	getrect.c hmenuhit.c popen.c printf.c put.c rdwr.c

$L:	$(OBJ)
	$(AR) uv $L $^


new:	$(OBJ)
		$(AR) uv $L `mlorder $(FILES) | tsort`
		$(AR) ma clip.o $L line.o
		rm $(FILES)

print:
	@pr $(CFILES)

pp:
	pp -t'Mux Graphics' -kjerq $(CFILES) | $(DCAN)

lint:
	lint -h -c -u -n -I/usr/jerq/include $(CFILES)

$L(clip.o):	$(INCL)/line.h
$L(comm.o):	$(INCL)/jerqio.h
$L(data.o):	$(INCL)/jerqio.h
$L(defont.o):	$(INCL)/font.h
$L(exit.o):	$(INCL)/jerqio.h
$L(fgets.o):	$(INCL)/jerqio.h
$L(font.o):	$(INCL)/jerqio.h
$L(font.o):	$(INCL)/font.h
$L(fopen.o):	$(INCL)/jerqio.h
$L(get.o):	$(INCL)/jerqio.h
$L(getfont.o):	$(INCL)/font.h
$L(getfont.o):	$(INCL)/jerqio.h
$L(jlineto.o):	$(INCL)/line.h
$L(jstring.o):	$(INCL)/font.h
$L(line.o):	$(INCL)/line.h
$L(menu.o):	$(INCL)/font.h
$L(popen.o):	$(INCL)/jerqio.h
$L(printf.o):	$(INCL)/jerqio.h
$L(put.o):	$(INCL)/jerqio.h
$L(rdwr.o):	$(INCL)/jerqio.h
$L(string.o):	$(INCL)/font.h
$L(strwidth.o):	$(INCL)/font.h
$L(wait.o):	$(INCL)/queue.h

.PRECIOUS:	$L

