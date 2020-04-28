CFLAG = -DDEBUG -Wall -std=c99

project_1: project_1.o 
	gcc $(CFLAG) project_1.o -o project_1
project_1.o: project_1.c Makefile
	gcc $(CFLAG) project_1.c -c
clean:
	rm -rf *o
run:
	sudo ./project_1
