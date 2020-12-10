source-files:=./bytecode_optimisations/dummy.c ./bytecode_optimisations/transformation.c ./bytecode_optimisations/record_changes.c ./bytecode_optimisations/function_inlining.c ./bytecode_optimisations/bytecode_manipulation.c ./bytecode_optimisations/predict_instruction_output.c ./bytecode_optimisations/bytecode_optimisation_utility.c ./bytecode_optimisations/bytecode_optimisations.c ./containers/stream.c ./containers/objects_vector.c ./containers/stack.c ./containers/vector.c ./containers/map.c ./error/execution_state.c ./error/error.c ./runtime/builtins.c ./runtime/struct_descriptor.c ./runtime/import_dll.c ./execution/options.c ./execution/debugger.c ./execution/executor.c ./execution/execution.c ./execution/repl.c ./callables/coroutine.c ./callables/function.c ./utility.c ./bytecode/execute_bytecode.c ./bytecode/bytecode_program.c ./bytecode/bytecode.c ./ast/macros.c ./ast/ast_optimisations.c ./ast/execute_ast.c ./transformers/expression_object.c ./transformers/ast_to_bytecode.c ./transformers/bytecode_to_lightning_previous.c ./transformers/bytecode_to_lightning.c 
 object-files:=dummy.o transformation.o record_changes.o function_inlining.o bytecode_manipulation.o predict_instruction_output.o bytecode_optimisation_utility.o bytecode_optimisations.o stream.o objects_vector.o stack.o vector.o map.o execution_state.o error.o builtins.o struct_descriptor.o import_dll.o options.o debugger.o executor.o execution.o repl.o coroutine.o function.o utility.o execute_bytecode.o bytecode_program.o bytecode.o macros.o ast_optimisations.o execute_ast.o expression_object.o ast_to_bytecode.o bytecode_to_lightning_previous.o bytecode_to_lightning.o 
HAS_LIGHTNING = 1
submodules := object_system/object_system.a parser/parser.a
build-folder := ../build
CC := gcc

options := -g3 -gdwarf-2 -Wall -Wl,--out-implib,$(build-folder)/libhost.a -D DEBUGGING
ifdef HAS_LIGHTNING
	options:= -llightning /home/piotr/Programming/lightning-2.1.2/lib/liblightning.la -D HAS_LIGHTNING $(options)
	CC:= libtool --mode=link $(CC)
endif
# linux requires -ldl flag
ifneq ($(OS),Windows_NT)
	options:=$(options) -ldl
endif

host-tests := tests
executable := duck
tests := tests.dk
tests-path := $(build-folder)/$(tests)
executable-path := $(build-folder)/$(executable)
host-tests-path := $(build-folder)/$(host-tests)

all: $(executable-path)
	cd $(build-folder)/; ./$(executable)

tests: $(executable-path) .//$(tests-path)
	cd $(build-folder)/; ./$(executable) ./$(tests)

repl: $(executable-path)
	cd $(build-folder)/; ./$(executable) -repl

host-tests: $(build-folder)/$(host-tests)
	cd $(build-folder)/; ./$(host-tests)

object_system/object_system.a:
	make -C object_system

parser/parser.a:
	make -C parser

$(executable-path): main.c $(source-files) $(submodules)
	$(CC) -o $(executable-path) main.c $(source-files) $(submodules) $(options)

$(host-tests-path): tests.c $(source-files) $(submodules)
	$(CC) -o $(host-tests-path) tests.c $(source-files) $(submodules) $(options)

clean:
	rm $(executable-path) $(host-tests-path)