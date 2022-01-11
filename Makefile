CC = gcc -Iinclude -w
all:cassini saturnd 

saturnd:src/saturnd.o
	gcc src/saturnd.o -o saturnd 

saturnd.o:src/saturnd.c 
	gcc -c src/saturnd.c 

time-comp.o:src/time-comp.c
	gcc -c src/time-comp.c
utils.o: src/utils.c
	gcc -c src/utils.c -Iinclude

cassini:src/cassini.o
	gcc src/cassini.o -o cassini

cassini.o:src/cassini.c
	gcc -c src/cassini.c 
distclean:
	rm -f src/*.o
