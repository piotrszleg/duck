all: object_system.a

tests: tests.exe
	./tests.exe

# compile c files into objects
map.o object.o tests.o error.o: map.c map.h object.c object.h error.h error.c vector.h vector.c
	gcc -c map.c object.c error.c vector.c

# pack object files into archive
object_system.a: map.o object.o error.o vector.o
	ar cr object_system.a map.o object.o error.o  vector.o

# compile c files into exe
tests.exe: map.c map.h object.c object.h tests.c error.h error.c vector.h vector.c
	gcc -Wall -o tests.exe map.c object.c error.c tests.c vector.c

clean:
	rm object_system.a
	rm error.o map.o object.o tests.o
	rm test.exe