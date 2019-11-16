sources := sample_dll.c
objects := sample_dll.o
resulting-library := sample.dll
options := -L. -lhost

all: $(objects)
	gcc -shared -o $(resulting-library) $(objects) $(options)

$(objects): $(sources)
	gcc -c $(sources)