OFILES= doprint.o print.o pow10.o
CFLAGS	= -O
LIB	= libv9.a

$(LIB): $(OFILES)
	ar cr $(LIB) $(OFILES)
	ranlib $(LIB)

veryclean: clean
	-rm $(LIB)

clean:
	-rm *.o
