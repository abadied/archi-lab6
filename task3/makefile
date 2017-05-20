all: myshell

myshell: myshell.o LineParser.o JobControl.o
	gcc -g -m32 -Wall -o myshell myshell.o LineParser.o JobControl.o
myshell.o: myshell.c
	gcc -m32 -g -Wall -ansi -c -Iinclude -o myshell.o myshell.c
.PHONY: clean

LineParser.o: LineParser.c
	gcc -m32 -g -Wall -c -Iinclude -o LineParser.o LineParser.c

JobControl.o: JobControl.c
	gcc -m32 -g -Wall -c -Iinclude -o JobControl.o JobControl.c

clean:
	rm -f *.o myshell
                                    