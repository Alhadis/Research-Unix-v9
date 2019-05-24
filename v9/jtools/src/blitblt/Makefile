CC=cc
CFLAGS=-g -I../sunlib -DSUNTOOLS -DBSD
HOSTCC=cc
LIBS= ../sunlib/libsj.a -lsuntool -lsunwindow -lpixrect

all: blitblt

BBTERM=blitblt.o flash.o sendbitmap.o reshape.o button.o \
       buttons.o kbdstring.o mice.o

blitblt:  $(BBTERM)
	$(CC) $(CFLAGS) -o $@ $(BBTERM) $(LIBS)

clean:
	rm -f $(BBTERM) blitblt
