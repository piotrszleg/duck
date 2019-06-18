submodules := object_system/object_system.a parser/parser.a

source-files := bytecode.c ast_to_bytecode.c execute_bytecode.c execute_ast.c \
error/execution_state.c execution.c runtime/builtins.c \
optimisations/ast_optimisations.c optimisations/bytecode_optimisations.c \
datatypes/stream.c datatypes/vector.c runtime/struct_descriptor.c repl.c runtime/import_dll.c options.c utility.c macros.c

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

$(executable-path): main.c $(source-files) $(submodules)
	gcc -o $(executable-path) main.c $(source-files) $(submodules) $(options)

$(tests-path): tests.c $(source-files) $(submodules)
	gcc -o $(tests-path) tests.c $(source-files) $(submodules) $(options)

$(sandbox-path): sandbox.c $(source-files) $(submodules)
	gcc -o $(sandbox-path) sandbox.c $(source-files) $(submodules)

clean:
	rm $(executable-path) $(tests-path)