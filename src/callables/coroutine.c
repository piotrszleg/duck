#include "coroutine.h"

Object new_coroutine(Executor* E, Object function, Object* arguments, int arguments_count) {
    REQUIRE_TYPE(function, t_function)
    REQUIRE(function.fp->ftype==f_bytecode, function)
    REQUIRE(function.fp->arguments_count==arguments_count, function)

    Object coroutine;
    coroutine_init(E, &coroutine);
    
    Executor* CE=malloc(sizeof(Executor));

    // copy bytecode program
    CE->options=E->options;
    bytecode_environment_init(&CE->bytecode_environment);
    vector_init(&CE->stack, sizeof(Object), 8);
    vector_init(&CE->traceback, sizeof(TracebackPoint), 16);
    // coroutine executor shares garbage collector and symbols with main executor
    *OBJECT_SYSTEM(CE)=*OBJECT_SYSTEM(E);

    // create a coroutine scope inheriting from global scope
    table_init(CE, &CE->scope);
    inherit_scope(CE, CE->scope, get(E, E->scope, to_string("builtins")));

    CE->coroutine=coroutine.co;
    coroutine.co->state=co_uninitialized;

    // pass arguments and move to given function but don't call it yet
    CE->bytecode_environment.executed_program=(BytecodeProgram*)function.fp->source_pointer;
    heap_object_reference((HeapObject*)function.fp->source_pointer);
    
    for(int i=0; i<arguments_count; i++) {
        objects_vector_push(&CE->stack, arguments[i]);
    }

    coroutine.co->executor=CE;
    return coroutine;
}

void coroutine_foreach_children(Executor* E, Coroutine* co, ForeachChildrenCallback callback, void* data){
    executor_foreach_children(E, co->executor, callback, data);
}

void coroutine_free(Coroutine* co){
    free(co->executor);
    free(co);
}

Object call_coroutine(Executor* E, Coroutine* coroutine, Object* arguments, int arguments_count){
    switch(coroutine->state){
        case co_uninitialized:
            if(arguments_count!=0){
                RETURN_ERROR("COROUTINE_ERROR", wrap_heap_object((HeapObject*)coroutine), "First call to coroutine shouldn't have any arguments, %i given", arguments_count)
            } else {
                coroutine->state=co_running;
                return execute_bytecode(coroutine->executor);
            }
        case co_running:
            // coroutine expects one value to be emitted from yield call
            if(arguments_count==1){
                objects_vector_push(&coroutine->executor->stack, arguments[0]);
            } else if(arguments_count==0){
                objects_vector_push(&coroutine->executor->stack, null_const);
            } else {
                RETURN_ERROR("COROUTINE_ERROR", wrap_heap_object((HeapObject*)coroutine), "Coroutines can accept either zero or one argument, %i given", arguments_count)
            }
            return execute_bytecode(coroutine->executor);
        case co_finished: return null_const;
        default: RETURN_ERROR("COROUTINE_ERROR", wrap_heap_object((HeapObject*)coroutine), "Unknown coroutine state: %i", coroutine->state)
    }
}