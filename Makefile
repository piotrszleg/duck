submodules := object_system/object_system.a parser/parser.a

headers := bytecode.h ast_to_bytecode.h execute_bytecode.h execute_ast.h \
error/execution_state.h execution.h runtime/builtins.h \
optimisations/ast_optimisations.h optimisations/bytecode_optimisations.h \
datatypes/stream.h datatypes/stack.h runtime/struct_descriptor.h runtime/import_dll.h options.h utility.h

source-files := bytecode.c ast_to_bytecode.c execute_bytecode.c execute_ast.c \
error/execution_state.c execution.c runtime/builtins.c \
optimisations/ast_optimisations.c optimisations/bytecode_optimisations.c \
datatypes/stream.c datatypes/stack.c runtime/struct_descriptor.c repl.c runtime/import_dll.c options.c utility.c

tests-path := tests.exe
options := -g -Wall -Wl,--out-implib,libhost.a -Wl,--export-all-symbols
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
	gcc -o $(executable-path) main.c $(source-files) $(submodules) $(options)

$(tests-path): tests.c $(source-files) $(headers) $(submodules)
	gcc -o $(tests-path) tests.c $(source-files) $(submodules) $(options)

$(sandbox-path): sandbox.c $(source-files) $(headers) $(submodules)
	gcc -o $(sandbox-path) sandbox.c $(source-files) $(submodules)

clean:
	rm $(executable-path) $(tests-path)