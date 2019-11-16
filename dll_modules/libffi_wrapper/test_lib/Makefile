sources := test_lib.c
objects := test_lib.o
resulting-library := test_lib.dll
options := -g -L. -lhost

all: $(objects)
	gcc -shared -o $(resulting-library) $(objects) $(options)
	rm $(objects)

$(objects): $(sources)
	gcc -c $(sources)