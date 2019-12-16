#include "executor.h"

Object executor_on_unhandled_error(Executor* E, Object error) {
    if(E->scope.type==t_table){
        Object handler=get(E, E->scope, to_string("on_unhandled_error"));
        if(handler.type!=t_null){
            return call(E, handler, &error, 1);
        }
    }
    USING_STRING(stringify(E, error),
        printf("Unhandled error:\n%s", str));
    return null_const;
}

void executor_foreach_children(Executor* E, Executor* iterated_executor, ManagedPointerForeachChildrenCallback callback){
    vector* return_stack=&E->bytecode_environment.return_stack;
    for(int i=0; i<vector_count(return_stack); i++){
        ReturnPoint* return_point=vector_index(return_stack, i);
        callback(E, &return_point->scope);
        if(return_point->program!=NULL){
            Object wrapped_program=wrap_heap_object((HeapObject*)return_point->program);
            callback(E, &wrapped_program);
        }
    }
    for(int i=0; i<vector_count(&E->stack); i++){
        callback(E, (Object*)vector_index(&E->stack, i));
    }
    callback(E, &E->scope);
    if(E->bytecode_environment.executed_program!=NULL){
        Object wrapped_program=wrap_heap_object((HeapObject*)E->bytecode_environment.executed_program);
        callback(E, &wrapped_program);
    }
}

Object executor_get_patching_table(Executor* E){
    if(E->scope.type==t_table){
        return table_get(E, E->scope.tp, to_string("patching_table"));
    } else {
        return null_const;
    }
}

void executor_collect_garbage(Executor* E){
    gc_unmark_all(E->object_system.gc);
    executor_foreach_children(E, E, gc_mark);
    gc_sweep(E);
    
}

void executor_init(Executor* E){
    E->options=default_options;
    E->scope=null_const;
    E->file=NULL;
    E->line=0;
    E->column=0;
    E->coroutine=NULL;
    E->returning=false;
    vector_init(&E->traceback, sizeof(TracebackPoint), 16);
    object_system_init(E);
    bytecode_environment_init(&E->bytecode_environment);
    vector_init(&E->stack, sizeof(Object), 8);
}

void executor_deinit(Executor* E){
    E->scope=null_const;
    object_system_deinit(E);
    vector_deinit(&E->traceback);
    bytecode_environment_deinit(&E->bytecode_environment);
    vector_deinit(&E->stack);
}