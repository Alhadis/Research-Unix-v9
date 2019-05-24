LIBDIR=/usr/lib

libv9.a:
	ar cr libv9.a *.o
	ranlib libv9.a

install: libv9.a
	cp libv9.a $(LIBDIR)
	ranlib $(LIBDIR)/libv9.a

clean:
	rm -f *.o
