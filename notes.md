## TODO:
- resize stack
- stringify override
- move execution state into a struct
- create coroutines
- more comments

## POINTERS:
**&** - returns pointer to it's operand
    `operand` becomes `operand*`
    `operand*` becomes `operand**`
    and so on
    watch out for macro expansion

**\*** - when it appears after type name it transforms it into pointer type 
    `int* p;` means p is a of type "pointer to int" 
    but people who designed c generously decided to saved us from remembering too much symbols so the same symbol is used in three completely different contexts, how fun. So when * is used with a pointer it dereferences it 
    `*operand` becomes `operand`, `**operand` becomes `*operand` 
    and `*operand` becomes a deadly bug in your code if you're using `void`  pointers 

[Linking libraries dynamically](https://github.com/alainfrisch/flexdll)
[Assembly guide](http://www.cs.virginia.edu/~evans/cs216/guides/x86.html)