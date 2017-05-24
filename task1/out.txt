all: task1

task1: task1.o LineParser.o 
	gcc -g -m32 -Wall -o task1 task1.o LineParser.o 
task1.o: task1.c
	gcc -m32 -g -Wall -ansi -c -Iinclude -o task1.o task1.c
.PHONY: clean

LineParser.o: LineParser.c
	gcc -m32 -g -Wall -c -Iinclude -o LineParser.o LineParser.c

clean:
	rm -f *.o task1
                                    