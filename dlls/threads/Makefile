sources := threads.c
objects := threads.o
resulting-library := threads.dll
options := -g -Wall -L. -lpthreadGC2 -lhost -Wl,--export-all-symbols

all: $(objects)
	gcc -shared -o $(resulting-library) $(objects) $(options)
	rm $(objects)

$(objects): $(sources)
	gcc -c $(sources) $(options)

clean:
	rm $(objects)