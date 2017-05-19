all: mypipe

mypipe: mypipe.o
	gcc -g -m32 -Wall -o mypipe mypipe.o
mypipe.o: mypipe.c
	gcc -m32 -g -Wall -ansi -c -Iinclude -o mypipe.o mypipe.c
.PHONY: clean

clean:
	rm -f *.o mypipe