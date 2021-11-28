CC = gcc -Iinclude
all:cassini
cassini:src/cassini.o
	gcc src/cassini.o -o cassini

cassini.o:src/cassini.c
	gcc -c src/cassini.c 
distclean:
	rm -f src/*.o
