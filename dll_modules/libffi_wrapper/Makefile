sources := libffi_wrapper.c
objects := libffi_wrapper.o
include_paths = -IE:/Libraries/libffi-3.2.1/i686-pc-mingw32/include
library_paths = -LE:/Libraries/libffi-3.2.1/i686-pc-mingw32/.libs
resulting-library := libffi_wrapper.dll
options := -g3  -gdwarf-2 -Wall -L. -lhost -lffi -Wl,--export-all-symbols

all: $(objects)
	gcc -shared -o $(resulting-library) $(objects) $(include_paths) $(library_paths) $(options)
	rm $(objects)

$(objects): $(sources)
	gcc -c $(sources) $(include_paths) $(library_paths) $(options)