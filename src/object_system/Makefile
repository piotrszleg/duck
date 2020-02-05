archive-path := object_system.a
tests-path := tests.exe
source-files := ../error/error.c $(shell find . ! -name "tests.c" -name "*.c")
executable-only-source-files := ../containers/stream.c  ../containers/vector.c ../utility.c
object-files := $(shell find . -name "*.o")

all: $(archive-path)

tests: $(tests-path)
	./$(tests-path)

# compile c files into objects
compile-objects: $(source-files)
	gcc -g -Wall -c $(source-files)

# pack object files into archive that can be added to main executable
$(archive-path): compile-objects
	ar cr $(archive-path) $(object-files)

# compile c files into exe with testing functions
$(tests-path): tests.c $(source-files) $(executable-only-source-files)
	gcc -g -Wall -o $(tests-path) tests.c $(source-files) $(executable-only-source-files)

clean:
	rm -f $(archive-path) $(tests-path) $(object-files)