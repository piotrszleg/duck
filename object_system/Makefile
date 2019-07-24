archive-path := object_system.a
tests-path := tests.exe
headers := object.h table.h ../error/error.h ../datatypes/stream.h object_operations.h error_object.h pipe_object.h binding_object.h object_utility.h
source-files := ../error/error.c object.c table.c object_operations.c error_object.c pipe_object.c binding_object.c object_utility.c
executable-only-source-files := ../datatypes/stream.c ../utility.c
object-files := error.o object.o table.o object_operations.o error_object.o pipe_object.o binding_object.o object_utility.o

all: $(archive-path)

tests: $(tests-path)
	./$(tests-path)

# compile c files into objects
$(object-files): $(source-files) $(headers)
	gcc -g -Wall -c $(source-files)

# pack object files into archive that can be added to main executable
$(archive-path): $(object-files)
	ar cr $(archive-path) $(object-files)
	rm $(object-files)

# compile c files into exe with testing functions
$(tests-path): tests.c $(source-files) $(executable-only-source-files) $(headers)
	gcc -g -Wall -o $(tests-path) tests.c $(source-files) $(executable-only-source-files)

clean:
	rm $(archive-path) $(tests-path) $(object-files)