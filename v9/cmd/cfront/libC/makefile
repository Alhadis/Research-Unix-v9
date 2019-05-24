install:
	cd complex;	CC -c *.c
	cd ../stream;	CC -c *.c
	cd ../task;	CC -c *.[cs]
	cd ../new;	CC -c *.c
	cd ../misc;	CC -c *.c
	cd ../generic;	CC -c *.c
	ar cr xx */*.[oi]
	mv xx /lib/libC.a
clean:
	rm -f */*.[oi] xx
