CC=cc
CFLAGS=-DX11 -g -I../../sunlib
LIBS= ../../sunlib/libj.a -lX11 -lipc
swar:	swar.o ToBitmap.o
	${CC} $(CFLAGS) swar.o ToBitmap.o ${LIBS} -o swar
	strip -s swar
swar.o:	player0.icon player1.icon accel.h missile.icon deathstar.icon boom.icon

clean:
	rm -f *.o m.out core swar
