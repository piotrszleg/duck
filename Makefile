submodules := object_system/object_system.a parser/parser.a
headers := ast_executor.h
source-files := bytecode.c ast_executor.c
executable-path := duck.exe
tests-path := tests.exe

all: $(executable-path) input
	./$(executable-path) input

repl: $(executable-path)
	./$(executable-path)

tests: $(tests-path)
	./$(tests-path)

object_system/object_system.a:
	make -C object_system

parser/parser.a:
	make -C parser

$(executable-path): main.c $(source-files) $(headers) $(submodules)
	gcc -g -Wall -o $(executable-path) main.c repl.c $(source-files) $(submodules)

$(tests-path): tests.c $(source-files) $(headers) $(submodules)
	gcc -g -Wall -o $(tests-path) tests.c $(source-files) $(submodules)

clean:
	rm $(executable-path) $(tests-path)