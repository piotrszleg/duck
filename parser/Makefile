all: parser.a

tests: tests.exe
	./tests.exe

# bison and lex
parser.tab.c parser.tab.h: parser.y
	bison -d parser.y

lex.yy.c: lexer.l parser.tab.h
	flex lexer.l

# compile c files into objects
parser.tab.o lex.yy.o ast.o vector.o: parser.tab.c parser.tab.h lex.yy.c ast.h ast.c vector.h vector.c
	gcc -g -Wall -c parser.tab.c lex.yy.c ast.c vector.c

# pack object files into archive
parser.a: parser.tab.o lex.yy.o vector.o ast.o
	ar cr parser.a parser.tab.o lex.yy.o vector.o ast.o
	rm parser.tab.o lex.yy.o vector.o ast.o

# compile c files into exe
tests.exe: parser.tab.c parser.tab.h lex.yy.c ast.h ast.c vector.h vector.c tests.c
	gcc -g -Wall -o tests.exe parser.tab.c lex.yy.c ast.c vector.c tests.c

clean:
	rm parser.a
	rm test.exe
	rm parser.tab.o lex.yy.o vector.o ast.o