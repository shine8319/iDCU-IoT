RASCC=arm-linux-gnueabi-gcc
GCC=gcc

cal:
	$(GCC) -fPIC -c sum.c sum2.c
	$(GCC) -shared -Wl,-soname,libmycalcso.so.0 -o libmycalcso.so.1.0.1 sum.o sum2.o
	ln -s libmycalcso.so.1.0.1 libmycalcso.so
	ln -s libmycalcso.so.1.0.1 libmycalcso.so.0
	$(GCC) -o cal.o cal.c -L./ -lmycalcso

clean:
	rm *.o *.*~ *.so* -f
