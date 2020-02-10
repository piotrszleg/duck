sources := regex.c
objects := regex.o
resulting-library := duck_regex.dll
options := -g3  -gdwarf-2 -Wall -L. -lregex2 -lhost -Wl,--export-all-symbols

all: $(objects)
	gcc -shared -o $(resulting-library) $(objects) $(options)
	rm $(objects)

$(objects): $(sources)
	gcc -c $(sources) $(options)