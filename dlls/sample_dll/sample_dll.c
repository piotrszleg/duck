#include "sample_dll.h"

float add_two(float x){
    return x+2;
}

Object duck_module_init(Executor* E){
    return to_string(strdup("Hello from duck_module_init!"));
}