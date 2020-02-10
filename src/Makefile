submodules := object_system/object_system.a parser/parser.a

source-files := \
	myjit/jitlib-core.c \
	$(shell find . \
		! -name "tests.c" \
		! -name "main.c" \
		! -path "./object_system*" \
		! -path "./parser*" \
		! -path "./myjit*" \
		-name "*.c")

build-folder := ../build

options := -g3 -gdwarf-2 -Wall -DDEBUGGING -Wl,--out-implib,$(build-folder)/libhost.a
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