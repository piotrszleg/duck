# Duck Programming Language

## About
Duck is a high level programming language with features similar to Python, Javascript and Lua. It is written in C and as for now it can be evaluated in three levels: abstract syntax tree, bytecode and machine code using MyJIT.

## Building
You'll need to have gcc compiler and make in your PATH. Type `gcc -v` and `make -v` into your terminal to make sure that they are accessible.

Type `make -C src` to build the project. If everything works out you'll have an executable file in `build/duck.exe`.

## Testing
All interpreter tests are contained in `tests` folder. In this folder there is also `tests` shell script that will go through all of them and print the results. 

## Usage
If you execute the interpreter without any arguments it'll enter read eval print loop. To evaluate a file add it as an argument, for example: `build/duck.exe input.dk`. For more options type in  `build/duck.exe -?`. For example programs written in Duck see `scripts` folder.

The language has a built in documentation in the form of `help` function that takes one argument. For example to see help for `print` function open the duck interpreter in repl mode and type in `help(print)` and press enter.

## DLLs
The main interpreter executable has no external dependencies to simplify the build process. The `dll_modules` folder contains C modules with optional functionality. 

To compile a dll_module copy `build/libhost.a` into its folder, then move to its folder and run `make`. 

The `.dk` files that test their functionality should be in the same folder. To run them you'll need to copy the resulting `.dll` files as well as their dependency `.dll` files into the same folder as the interpreter executable. If you installed the dependency `dlls` into your system's dynamic library search path (for example by using some linux package manager) you don't need to copy them into interpreter executable folder, only the `dlls` resulting from building the optional modules. The DLL dependencies for Windows are in `dlls` folder, for other systems you'll need to provide them yourself.