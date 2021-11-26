all:cassini
cassini:src/cassini.o
	gcc src/cassini.o -o cassini

cassini.o:src/cassini.c include/cassini.h
	gcc -c src/cassini.c
distclean:
	rm -f src/*.o
