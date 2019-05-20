submodules := object_system/object_system.a parser/parser.a

headers := bytecode.h ast_to_bytecode.h execute_bytecode.h execute_ast.h \
error/execution_state.h execution.h runtime/builtins.h \
optimisations/ast_optimisations.h optimisations/bytecode_optimisations.h \
datatypes/stream.h datatypes/stack.h runtime/struct_descriptor.h

source-files := bytecode.c ast_to_bytecode.c execute_bytecode.c execute_ast.c \
error/execution_state.c execution.c runtime/builtins.c \
optimisations/ast_optimisations.c optimisations/bytecode_optimisations.c \
datatypes/stream.c datatypes/stack.c runtime/struct_descriptor.c repl.c

tests-path := tests.exe
compiler-options := -g -Wall
executable-path := duck.exe
sandbox-path := sandbox.exe

all: $(executable-path) input
	./$(executable-path) input

tests: $(tests-path)
	./$(tests-path)

repl: $(executable-path)
	./$(executable-path)

sandbox: $(sandbox-path)
	./$(sandbox-path)

object_system/object_system.a:
	make -C object_system

parser/parser.a:
	make -C parser

$(executable-path): main.c $(source-files) $(headers) $(submodules)
	gcc $(compiler-options) -o $(executable-path) main.c $(source-files) $(submodules)

$(tests-path): tests.c $(source-files) $(headers) $(submodules)
	gcc $(compiler-options) -o $(tests-path) tests.c $(source-files) $(submodules)

$(sandbox-path): sandbox.c $(source-files) $(headers) $(submodules)
	gcc $(compiler-options) -o $(sandbox-path) sandbox.c $(source-files) $(submodules)

clean:
	rm $(executable-path) $(tests-path)