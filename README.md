# Duck Programming Language

## About
Duck is a high level programming language with features similar to Python, Javascript and Lua. It is written in C and as for now it can be evaluated in three levels: abstract syntax tree, bytecode and machine code using GNU lightning.

Expressions can be separated either by a newline or a comma and each expression carries a value. Iteration is realized using `#` and `##` binary operators that take iterable as a left side and function accepting one or two arguments as a right side. Additionally special builtin functions like `range` and `while` are provided.
 
Main Duck features are:
- dynamic typing
- automatic garbage collection
- monkey patching of builtin types
- AST level procedural macros
- table type providing:
  - string representation
  - iteration
  - hashing
  - deep copying
  - deep comparing
  - special keys
  - prototype based inheritance

## Examples

Checking if a number is even:
```
number=10
if((number%2)==0){
    print("even")
} else {
    print("not even")
}
```

Creating a table and printing its elements:
```
array=[1, 0.5, 'test, count=3]
// ## is entries iteration operator
// and >< is partial application operator
array ## (printf><"array[{}]={}")
```

Overriding text representation of an object:
```
// this object will be printed as "10$"
[
    value=10
    .[overrides.stringify]=self->format("{}$", self.value)
]
```

Printing object representation of a table expression using a macro:
```
@print_expression=expression->print(expression)
// macro takes expressions following it as its arguments
@print_expression, [x=1, y=2]
```

See `scripts` and `tests` folders for more.

## Builtin Types

| Type      | Examples                      | Description
|:----------|:------------------------------|:-----------
| null      | `null`                        | used to represent nothing: empty value in a table, uninitialized variable, nothing returned by a function
| int       | `1`                           | used to represent integer numbers
| float     | `0.5`                         | used to represent real numbers
| function  | `->1`, `x->2*x`, `(a, b)->a+b`| part of the program that can be called with arguments along with its creation scope
| string    | `'a`, `'name`, `"John Smith"` | immutable object representing text or a single character
| table     | `[1, 2, 3]`, `[x=1, y=2]`     | complex type containing unsorted key-value pairs, supporting various functionality overrides through symbol keys
| coroutine | `coroutine(->yield(1))`       | running cooperative thread supporting iteration protocol
| symbol    | `new_symbol('Monday)`         | unique object with a text comment, equivalent of enum types in statically typed languages, also used for special table keys
| any       | `types.any`                   | wildcard type used in place of a specific object type (for example in pattern matching)

All of these types are listed in `types` global variable, so for example to access int type you'd write `types.int`. 
Casting is done using `cast` function, for example `cast(1, types.string)`, there are also helper functions `to_int`, `to_float` and `to_string`.

Missing type functionality can be monkey patched at runtime by adding functions to these type tables. So you might, for example, implement an int iterator that will go through its bits and use it with all of the standard functions and operators working on iterables. Here is a beginning of such code: `types.int[overrides.iterator]=value->...`.

## Building
You'll need to have gcc compiler and make in your PATH. Type `gcc -v` and `make -v` into your terminal to make sure that they are accessible.

Type `make -C src` to build the project. If everything works out you'll have an executable file in `build/duck.exe`.

If you want to use GNU lightning you'll have to install it, uncomment `HAS_LIGHTNING` in `src/Makefile` and correct its `.la` file path.

## Testing
All interpreter tests are contained in `tests` folder. In this folder there is also `tests` shell script that will go through all of them and print the results. 

## Usage
If you execute the interpreter without any arguments it'll enter read eval print loop. To evaluate a file add it as an argument, for example: `build/duck.exe input.dk`. For more options type in  `build/duck.exe -?`. For example programs written in Duck see `scripts` folder.

The language has a built in documentation in the form of `help` function that takes one argument. For example to see help for `print` function open the duck interpreter in repl mode and type in `help(print)` and press enter. List of all builtin functions and objects is available as `builtins` global variable.

## Optional Functionality
The `dll_modules` folder contains C modules with optional functionality. 

To compile a dll_module copy `build/libhost.a` into its folder, then move to its folder and run `make`. 

The `.dk` files that test their functionality should be in the same folder. To run them you'll need to copy the resulting `.dll` files as well as their dependency `.dll` files into the same folder as the interpreter executable. If you installed the dependency `dlls` into your system's dynamic library search path (for example by using some linux package manager) you don't need to copy them into interpreter executable folder, only the `dlls` resulting from building the optional modules. The DLL dependencies for Windows are in `dlls` folder, for other systems you'll need to provide them yourself.