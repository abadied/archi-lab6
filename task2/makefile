all: task2

task2: task2.o 
	gcc -g -m32 -Wall -o task2 task2.o  
task2.o: task2.c
	gcc -m32 -g -Wall -ansi -c -Iinclude -o task2.o task2.c
.PHONY: clean

clean:
	rm -f *.o task2