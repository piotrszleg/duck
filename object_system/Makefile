archive-path := object_system.a
tests-path := tests.exe
headers := object.h table.h ../error/error.h ../datatypes/stream.h object_operations.h error_object.h pipe_object.h
source-files := ../error/error.c object.c table.c ../datatypes/stream.c object_operations.c error_object.c pipe_object.c
object-files := error.o object.o table.o vector.o object_operations.o error_object.o pipe_object.o

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