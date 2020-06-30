source-files:=./ast/ast_optimisations.c ./ast/execute_ast.c ./ast/macros.c ./bytecode/bytecode.c ./bytecode/bytecode_program.c ./bytecode/execute_bytecode.c ./bytecode_optimisations/bytecode_manipulation.c ./bytecode_optimisations/bytecode_optimisations.c ./bytecode_optimisations/bytecode_optimisation_utility.c ./bytecode_optimisations/dummy.c ./bytecode_optimisations/function_inlining.c ./bytecode_optimisations/predict_instruction_output.c ./bytecode_optimisations/record_changes.c ./bytecode_optimisations/transformation.c ./callables/coroutine.c ./callables/function.c ./containers/map.c ./containers/objects_vector.c ./containers/stream.c ./containers/vector.c ./error/error.c ./error/execution_state.c ./execution/debugger.c ./execution/execution.c ./execution/executor.c ./execution/options.c ./execution/repl.c ./runtime/builtins.c ./runtime/import_dll.c ./runtime/struct_descriptor.c ./transformers/ast_to_bytecode.c ./transformers/bytecode_to_myjit.c ./transformers/expression_object.c ./utility.c 
 object-files:=ast_optimisations.o execute_ast.o macros.o bytecode.o bytecode_program.o execute_bytecode.o bytecode_manipulation.o bytecode_optimisations.o bytecode_optimisation_utility.o dummy.o function_inlining.o predict_instruction_output.o record_changes.o transformation.o coroutine.o function.o map.o objects_vector.o stream.o vector.o error.o execution_state.o debugger.o execution.o executor.o options.o repl.o builtins.o import_dll.o struct_descriptor.o ast_to_bytecode.o bytecode_to_myjit.o expression_object.o utility.o 
HAS_LIGHTNING = 1
submodules := object_system/object_system.a parser/parser.a
build-folder := ../build

options := -g3 -gdwarf-2 -Wall -Wl,--out-implib,$(build-folder)/libhost.a #-DDEBUGGING
ifdef $(HAS_LIGHTNING)
	options:=$(options)  -llightning -D HAS_LIGHTNING
endif
# linux requires -ldl flag
ifneq ($(OS),Windows_NT)
	options:=$(options) -ldl
endif

host-tests := tests.exe
executable := duck.exe
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
	gcc -o $(executable-path) main.c $(source-files) $(submodules) $(options)

$(host-tests-path): tests.c $(source-files) $(submodules)
	gcc -o $(host-tests-path) tests.c $(source-files) $(submodules) $(options)

clean:
	rm $(executable-path) $(host-tests-path)