archive-path := parser.a
tests-path := tests.exe
repl-path := repl.exe
headers := parser.tab.h ast.h ../datatypes/vector.h ../error/error.h
source-files := parser.tab.c lex.yy.c ast.c ../datatypes/vector.c ../error/error.c
object-files := parser.tab.o lex.yy.o ast.o vector.o

all: $(archive-path)

tests: $(tests-path)
	./$(tests-path)

repl: $(repl-path)
	./$(repl-path)

bison: lex.yy.c parser.tab.c parser.tab.h

# bison and lex
parser.tab.c parser.tab.h: parser.y
	bison -d parser.y

lex.yy.c: lexer.l parser.tab.h
	flex lexer.l

# compile c files into objects
$(object-files): $(source-files) $(headers)
	gcc -g -Wall -c $(source-files)

# pack object files into archive
$(archive-path): $(object-files)
	ar cr $(archive-path) $(object-files)

# compile c files into exe
$(tests-path): tests.c $(source-files) $(headers)
	gcc -g -Wall -o $(tests-path) tests.c $(source-files)

$(repl-path): repl.c $(source-files) $(headers)
	gcc -g -Wall -o $(repl-path) repl.c $(source-files)

clean:
	rm $(archive-path) $(tests-path) $(repl-path) $(object-files) parser.tab.c parser.tab.h lex.yy.c