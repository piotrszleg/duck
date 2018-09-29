all: object_system.a

map.o object.o tests.o error.o: map.c map.h object.c object.h tests.c tests.h error.h error.c
	gcc -c map.c object.c tests.c error.c

object_system.a: map.o object.o tests.o error.o
	ar cr object_system.a map.o object.o tests.o error.o

clean:
	rm object_system.a