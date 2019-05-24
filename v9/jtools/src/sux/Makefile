CFLAGS	= -I../sunlib -g -DBSD -DX11
LIBS	= ../sunlib/libj.a -lX11
OFILES	= window.o deltext.o frameop.o instext.o newframe.o \
	  ptofchar.o select.o string.o sux.o

sux: $(OFILES)
	cc -o sux $(CFLAGS) $(OFILES) $(LIBS)

