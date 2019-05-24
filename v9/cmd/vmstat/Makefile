CFLAGS = -O

vmstat:	vmstat.o
	cc -o vmstat vmstat.o

install:	vmstat
	strip vmstat
	cp vmstat /bin
	/etc/chown ,sys /bin/vmstat
	chmod g+s /bin/vmstat

clean:
	rm -f vmstat *.o
