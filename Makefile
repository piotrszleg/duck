submodules := object_system/object_system.a parser/parser.a

source-files := \
	bytecode.c \
	bytecode_program.c \
	ast_to_bytecode.c \
	execute_bytecode.c \
	execute_ast.c \
	error/execution_state.c \
	execution.c \
	runtime/builtins.c \
	optimisations/ast_optimisations.c \
	optimisations/bytecode_optimisations.c \
	optimisations/dummy.c \
	optimisations/transformation.c \
	datatypes/stream.c \
	datatypes/vector.c \
	runtime/struct_descriptor.c \
	repl.c \
	runtime/import_dll.c \
	options.c \
	utility.c \
	macros.c \
	coroutine.c \
	myjit/jitlib-core.c \
	bytecode_to_myjit.c\

host-tests-path := tests.exe
options := -g -Wall -Wl,--out-implib,libhost.a -IE:\Libraries\libffi-3.2.1/i686-pc-mingw32/include/
ifneq ($(OS),Windows_NT)
	options:=$(options) -ldl
endif
executable-path := duck.exe
sandbox-path := sandbox.exe

all: $(executable-path)
	./$(executable-path)

tests: $(executable-path) tests.dk
	./$(executable-path) tests.dk

host-tests: $(host-tests-path)
	./$(host-tests-path)

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

$(host-tests-path): tests.c $(source-files) $(submodules)
	gcc -o $(host-tests-path) tests.c $(source-files) $(submodules) $(options)

$(sandbox-path): sandbox.c $(source-files) $(submodules)
	gcc -o $(sandbox-path) sandbox.c $(source-files) $(submodules)

clean:
	rm $(executable-path) $(host-tests-path)