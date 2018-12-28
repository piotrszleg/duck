submodules := object_system/object_system.a parser/parser.a
headers := builtins.h bytecode.h ast_to_bytecode.h execute_bytecode.h execute_ast.h ast_optimisations.h ast_visitor.h datatypes/stream.h datatypes/stack.h
source-files := builtins.c bytecode.c ast_to_bytecode.c execute_bytecode.c execute_ast.c ast_optimisations.c ast_visitor.c datatypes/stream.c datatypes/stack.c
executable-path := duck.exe
tests-path := tests.duck
sandbox-path := sandbox.exe

all: $(executable-path) input
	./$(executable-path) input

repl: $(executable-path)
	./$(executable-path)

tests: $(executable-path) $(tests-path)
	./$(executable-path) $(tests-path)

sandbox: $(sandbox-path)
	./$(sandbox-path)

object_system/object_system.a:
	make -C object_system

parser/parser.a:
	make -C parser

$(executable-path): main.c $(source-files) $(headers) $(submodules)
	gcc -g -Wall -o $(executable-path) main.c repl.c $(source-files) $(submodules)

$(sandbox-path): sandbox.c $(source-files) $(headers) $(submodules)
	gcc -g -Wall -o $(sandbox-path) sandbox.c $(source-files) $(submodules)

clean:
	rm $(executable-path) $(tests-path)