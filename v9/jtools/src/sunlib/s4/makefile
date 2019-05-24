CFLAGS  = -g -I.. -DBSD -DSUNTOOLS
LIB     = libs4j.a
LIBS    = $(LIB) -lsuntool -lsunwindow -lpixrect
OFILES = add.o bitblt.o button.o circle.o cos.o jerq.o menuhit.o mhit.o\
	muxbuf.o wait.o snd.o kbdrcv.o rcvfill.o sizehints.o ToBitmap.o\
	sun4.o getpoint.o
TESTS = t0 t1 t2 t3 t4 t5

$(LIB): $(OFILES) ../jerq.h
	rm -f $(LIB)
	ar cr $(LIB) $(OFILES)
	ranlib $(LIB)

$(OFILES): ../jerq.h

all: 	$(LIB) $(TESTS)

add.o: ../add.c
	$(CC) -c $(CFLAGS) ../add.c
bitblt.o: ../bitblt.c
	$(CC) -c $(CFLAGS) ../bitblt.c
button.o: ../button.c
	$(CC) -c $(CFLAGS) ../button.c
circle.o: ../circle.c
	$(CC) -c $(CFLAGS) ../circle.c
cos.o: ../cos.c
	$(CC) -c $(CFLAGS) ../cos.c
getpoint.o: ../getpoint.c
	$(CC) -c $(CFLAGS) ../getpoint.c
jerq.o: ../jerq.c
	$(CC) -c $(CFLAGS) ../jerq.c
menuhit.o: ../menuhit.c
	$(CC) -c $(CFLAGS) ../menuhit.c
mhit.o: ../mhit.c
	$(CC) -c $(CFLAGS) ../mhit.c
muxbuf.o: ../muxbuf.c
	$(CC) -c $(CFLAGS) ../muxbuf.c
wait.o: ../wait.c
	$(CC) -c $(CFLAGS) ../wait.c
snd.o: ../snd.c
	$(CC) -c $(CFLAGS) ../snd.c
kbdrcv.o: ../kbdrcv.c
	$(CC) -c $(CFLAGS) ../kbdrcv.c
rcvfill.o: ../rcvfill.c
	$(CC) -c $(CFLAGS) ../rcvfill.c
sizehints.o: ../sizehints.c
	$(CC) -c $(CFLAGS) ../sizehints.c
ToBitmap.o: ../ToBitmap.c
	$(CC) -c $(CFLAGS) ../ToBitmap.c
sun4.o: ../sun4.c
	$(CC) -c $(CFLAGS) ../sun4.c

t0: ../t0.c $(LIB)
	$(CC) $(CFLAGS) -o t0 ../t0.c $(LIBS)
t1: ../t1.c $(LIB)
	$(CC) $(CFLAGS) -o t1 ../t1.c $(LIBS)
t2: ../t2.c $(LIB)
	$(CC) $(CFLAGS) -o t2 ../t2.c $(LIBS)
t3: ../t3.c $(LIB)
	$(CC) $(CFLAGS) -o t3 ../t3.c $(LIBS)
t4: ../t4.c $(LIB)
	$(CC) $(CFLAGS) -o t4 ../t4.c $(LIBS)
t5: ../t5.c $(LIB)
	$(CC) $(CFLAGS) -o t5 ../t5.c $(LIBS)
t6: ../t6.c $(LIB)
	$(CC) $(CFLAGS) -o t6 ../t6.c $(LIBS)
t7: ../t7.c $(LIB)
	$(CC) $(CFLAGS) -o t7 ../t7.c $(LIBS)
