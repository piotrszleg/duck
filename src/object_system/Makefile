source-files:=./object.c ./object_history.c ./object_utility.c ./operations/basic.c ./operations/copying.c ./operations/help.c ./operations/iteration.c ./operations/operators.c ./operations/patching.c ./operations/stringification.c ./prototype_chain.c ./special_objects/binding_object.c ./special_objects/error_object.c ./special_objects/pipe_object.c ./table.c ../error/error.c
object-files:=object.o object_history.o object_utility.o basic.o copying.o help.o iteration.o operators.o patching.o stringification.o prototype_chain.o binding_object.o error_object.o pipe_object.o table.o error.o 
archive-path := object_system.a
tests-path := tests.exe
executable-only-source-files := ../containers/stream.c  ../containers/vector.c ../utility.c
cc := gcc

all: $(archive-path)

tests: $(tests-path)
	./$(tests-path)

# compile c files into objects
compile-objects: $(source-files)
	$(cc) -g3  -gdwarf-2  -Wall -c $(source-files)

# pack object files into archive that can be added to main executable
$(archive-path): compile-objects
	ar cr $(archive-path) $(object-files)

# compile c files into exe with testing functions
$(tests-path): tests.c $(source-files) $(executable-only-source-files)
	$(cc) -g3  -gdwarf-2  -Wall -o $(tests-path) tests.c $(source-files) $(executable-only-source-files)

clean:
	rm -f $(archive-path) $(tests-path) $(object-files)