archive-path := object_system.a
tests-path := tests.exe
headers := map.h object.h ../error/error.h vector.h object_operations.h
source-files := ../error/error.c map.c object.c vector.c object_operations.c
object-files := error.o map.o object.o vector.o object_operations.o

all: $(archive-path)

tests: $(tests-path)
	./$(tests-path)

# compile c files into objects
$(object-files): $(source-files) $(headers)
	gcc -g -c $(source-files)

# pack object files into archive that can be added to main executable
$(archive-path): $(object-files)
	ar cr $(archive-path) $(object-files)

# compile c files into exe with testing functions
$(tests-path): tests.c $(source-files) $(headers)
	gcc -g -Wall -o $(tests-path) tests.c $(source-files)

clean:
	rm $(archive-path) $(tests-path) $(object-files)