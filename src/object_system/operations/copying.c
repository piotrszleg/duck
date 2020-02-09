#include "copying.h"

typedef struct {
    Table* visited;
    Table* identifier_to_copied;
} PlaceholdersReplacingState;

Object to_copy_replace_promise(int index){
    Object result;
    result.int_value=index;
    result.type=t_copy_replace_promise;
    return result;
}

bool is_copy_replace_promise(Object* o){
    return o->type==t_copy_replace_promise;
}

void replace_copy_placeholders(Executor* E, Object* o, void* data){
    PlaceholdersReplacingState* state=(PlaceholdersReplacingState*)data;
    if(is_heap_object(*o)){
        // this is to avoid infinite recursion
        if(table_get(E, state->visited, to_pointer(o->hp)).type!=t_null){
            return;
        }
        table_set(E, state->visited, to_pointer(o->hp), to_int(1));
        if(is_copy_replace_promise(o)) {
            *o=table_get(E, state->identifier_to_copied, *o);
        }
    }
}

void copying_state_init(Executor* E, CopyingState* state){
    state->identifiers_counter=0;

    Object copies;
    table_init(E, &copies);
    state->copies=copies.tp;

    Object identifier_to_copied;
    table_init(E, &identifier_to_copied);
    state->identifier_to_copied=identifier_to_copied.tp;
}

void placeholders_replacing_state_init(Executor* E, 
                                       PlaceholdersReplacingState* state, 
                                       Table* identifer_to_copied) {
    Object visited;
    table_init(E, &visited);
    state->visited=visited.tp;
    state->identifier_to_copied=identifer_to_copied;
    heap_object_reference((HeapObject*)state->identifier_to_copied);
}


void placeholders_replacing_state_deinit(Executor* E, 
                                       PlaceholdersReplacingState* state) {
    heap_object_dereference(E, (HeapObject*)state->visited);
    heap_object_dereference(E, (HeapObject*)state->identifier_to_copied);
}

void copying_state_deinit(Executor* E, CopyingState* state){
    heap_object_dereference(E, (HeapObject*)state->copies);
    heap_object_dereference(E, (HeapObject*)state->identifier_to_copied);
}

Object copy(Executor* E, Object o){
    if(is_heap_object(o)){
        CopyingState state;
        copying_state_init(E, &state);
        Object result=copy_recursive(E, o, &state);
        PlaceholdersReplacingState replacingState;
        placeholders_replacing_state_init(E, &replacingState, state.identifier_to_copied);

        heap_object_foreach_children(E, o.hp, replace_copy_placeholders, &replacingState);

        copying_state_deinit(E, &state);
        placeholders_replacing_state_deinit(E, &replacingState);
        return result;
    } else {
        // there can't be cycles in objects that dont't reside on heap
        return copy_recursive(E, o, NULL);
    }
}

Object copy_recursive(Executor* E, Object o, CopyingState* state){
    if(is_heap_object(o)){
        Object copies_field=table_get(E, state->copies, to_pointer(o.hp));
        if(copies_field.type==t_pointer){
            return wrap_heap_object((HeapObject*)copies_field.p);
        } else if(copies_field.type!=t_null) {
            return copies_field;// this field will be filled in later
        } else {
            state->identifiers_counter++;
            #define IDENTIFIER to_copy_replace_promise(state->identifiers_counter)
            table_set(E, state->copies, to_pointer(o.hp), IDENTIFIER);
        }
    }
    #define RETURN_HEAP_OBJECT(to_return) \
    { \
        Object to_return_temporary=to_return; \
        table_set(E, state->copies, to_pointer(o.hp), to_pointer(to_return_temporary.hp)); \
        table_set(E, state->identifier_to_copied, IDENTIFIER, to_pointer(to_return_temporary.hp)); \
        return to_return_temporary; \
    }
    
    switch(o.type){
        case t_float:
        case t_int:
        case t_null:
        case t_pointer:
            return o;
        case t_string:
        {
            return to_string(strdup(o.text));
        }
        case t_table:
        {
            Object copy_override=get(E, o, OVERRIDE(E, copy));
            if(copy_override.type!=t_null){
                RETURN_HEAP_OBJECT(call(E, copy_override, &o, 1));
            } else {
                RETURN_HEAP_OBJECT(table_copy(E, o.tp, state));
            }
        }
        case t_symbol:
        {
            Object copied;
            symbol_init(E, &copied);
            copied.sp->index=o.sp->index;
            copied.sp->comment=strdup(o.sp->comment);
            return copied;
        }
        case t_function:
        {
            Object copied;
            function_init(E, &copied);
            copied.fp->arguments_count=o.fp->arguments_count;
            if(o.fp->argument_names==NULL){
                copied.fp->argument_names=NULL;
            } else {
                copied.fp->argument_names=malloc(sizeof(char*)*copied.fp->arguments_count);
                for(int i=0; i<copied.fp->arguments_count; i++){
                    copied.fp->argument_names[i]=o.fp->argument_names[i];
                }
            }
            copied.fp->ftype=o.fp->ftype;
            switch(o.fp->ftype){
                case f_ast:
                case f_bytecode:{
                    Object copied_source_pointer=copy_recursive(E, wrap_heap_object((HeapObject*)o.fp->source_pointer), state);
                    if(is_error(E, copied_source_pointer)){
                        copied.fp->ftype=f_special;// to avoid deallocating source_pointer
                        dereference(E, &copied);
                        RETURN_ERROR("COPY_ERROR", MULTIPLE_CAUSES(o, copied_source_pointer), "Copying function's source pointer caused an error.");
                    } else if(!is_heap_object(copied_source_pointer)) {
                        copied.fp->ftype=f_special;
                        dereference(E, &copied);
                        RETURN_ERROR("COPY_ERROR", MULTIPLE_CAUSES(o, copied_source_pointer), "Result of copying function's source pointer isn't a heap object.");
                    } else {
                        copied.fp->source_pointer=copied_source_pointer.hp;
                        heap_object_reference(copied.fp->source_pointer);
                    }
                    break;
                }
                case f_native:
                    copied.fp->native_pointer=o.fp->native_pointer;
                    break;
                case f_special:
                    copied.fp->special_index=o.fp->special_index;
                    break;
            }
            copied.fp->enclosing_scope=copy_recursive(E, o.fp->enclosing_scope, state);
            if(is_error(E, copied.fp->enclosing_scope)){
                dereference(E, &copied);
                RETURN_ERROR("COPY_ERROR", MULTIPLE_CAUSES(o, copied.fp->enclosing_scope), "Copying function's scope pointer caused an error.");
            }
            reference(&copied.fp->enclosing_scope);
            RETURN_HEAP_OBJECT(copied);
        }
        case t_managed_pointer:
            if(o.mp->copy!=NULL) {
                return wrap_heap_object((HeapObject*)o.mp->copy(E, o.mp));
            } else {
                RETURN_ERROR("COPY_ERROR", null_const, "This managed pointer doesn't implement copying.");
            }
        default:
            RETURN_ERROR("COPY_ERROR", null_const, "Copying objects of type %s is not supported.", get_type_name(o.type));
    }
    #undef IDENTIFIER
}