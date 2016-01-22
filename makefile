make:	
	gcc -std=c99 add.c -g -o addtool
	ldd main
	export LD_LIBRARY_PATH=$(shell pwd)
clean:
	rm -rf main *.o *.so