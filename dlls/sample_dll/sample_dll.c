#include "sample_dll.h"

float add_two(float x){
    return x+2;
}

object duck_module_init(){
    return to_string(strdup("Hello from duck_module_init!"));
}