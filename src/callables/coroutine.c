#include "coroutine.h"

Object new_coroutine(Executor* E, Object function, Object* arguments, int arguments_count) {
    REQUIRE_TYPE(function, t_function)
    REQUIRE(function.fp->ftype==f_bytecode, function)
    REQUIRE(function.fp->arguments_count==arguments_count, function)

    Object coroutine;
    coroutine_init(E, &coroutine);
    
    Executor* coroutine_executor=malloc(sizeof(Executor));

    // copy bytecode program
    coroutine_executor->options=E->options;
    bytecode_environment_init(&coroutine_executor->bytecode_environment);
    vector_init(&E->scope, sizeof(Object), 8);
    vector_init(&coroutine_executor->traceback, sizeof(TracebackPoint), 16);
    // coroutine executor shares garbage collector and symbols with main executor
    *OBJECT_SYSTEM(coroutine_executor)=*OBJECT_SYSTEM(E);

    // create a coroutine scope inheriting from global scope
    table_init(coroutine_executor, &coroutine_executor->scope);
    inherit_scope(coroutine_executor, coroutine_executor->scope, get(E, E->scope, to_string("global")));

    coroutine_executor->coroutine=coroutine.co;
    coroutine.co->state=co_uninitialized;

    // pass arguments and move to given function but don't call it yet
    coroutine_executor->bytecode_environment.executed_program=(BytecodeProgram*)function.fp->source_pointer;
    heap_object_reference((HeapObject*)function.fp->source_pointer);
    
    for(int i=1; i<arguments_count; i++) {
        push(&coroutine_executor->bytecode_environment.object_stack, arguments[i]);
    }

    coroutine.co->executor=coroutine_executor;
    return coroutine;
}

void coroutine_foreach_children(Executor* E, Coroutine* co, ManagedPointerForeachChildrenCallback callback){
    executor_foreach_children(E, co->executor, callback);
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
                push(&coroutine->executor->bytecode_environment.object_stack, arguments[0]);
            } else if(arguments_count==0){
                push(&coroutine->executor->bytecode_environment.object_stack, null_const);
            } else {
                RETURN_ERROR("COROUTINE_ERROR", wrap_heap_object((HeapObject*)coroutine), "Coroutines can accept either zero or one argument, %i given", arguments_count)
            }
            return execute_bytecode(coroutine->executor);
        case co_finished: return null_const;
        default: RETURN_ERROR("COROUTINE_ERROR", wrap_heap_object((HeapObject*)coroutine), "Unknown coroutine state: %i", coroutine->state)
    }
}