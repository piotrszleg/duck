all: duck.exe
	./duck.exe input

repl: duck.exe
	./duck.exe

object_system/object_system.a:
	make -C object_system

parser/parser.a:
	make -C parser

duck.exe: main.c repl.c ast_executor.c ast_executor.h object_system/object_system.a parser/parser.a
	gcc -g -Wall -o duck.exe main.c repl.c ast_executor.c object_system/object_system.a parser/parser.a

clean:
	rm duck.exe