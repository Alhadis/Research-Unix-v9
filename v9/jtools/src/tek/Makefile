CFLAGS	= -I../sunlib -DBSD -DX11 -g 
LIBS	= ../sunlib/libj.a -lX11
OFILES	= jek.o noflash.o x11.o
tek: $(OFILES)
	cc -o tek $(OFILES) $(LIBS)
