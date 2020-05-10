#include "function.h"

// this function should only be called from call_function, it's there to simplify the code structure
static Object call_function_processed(Executor* E, Function* f, Object* arguments, int arguments_count){
    switch(f->ftype){
        case f_native:
            return f->native_pointer(E, f->enclosing_scope, arguments, arguments_count);
        case f_bytecode:
            create_return_point(E, true);
            for(int i=0; i<arguments_count; i++){
                objects_vector_push(&E->stack, arguments[i]);
                reference((Object*)vector_top(&E->stack));
            }
            move_to_function(E, f);
            BytecodeProgram* bytecode_program=(BytecodeProgram*)f->source_pointer;
            if(bytecode_program->compiled!=NULL){
                bytecode_program->compiled(E, bytecode_program);
                pop_return_point(E);
                return objects_vector_pop(&E->stack);
            } else {
                return execute_bytecode(E);
            }
        case f_ast: {
            Object function_scope;
            table_init(E, &function_scope);
            if(f->enclosing_scope.type!=t_null){
                inherit_scope(E, function_scope.tp, f->enclosing_scope);
            }
            for(int i=0; i<arguments_count; i++){
                table_set(E, function_scope.tp, to_string(f->argument_names[i]), arguments[i]);
            }
            vector_push(&E->stack, &E->scope);
            Object previous_scope=E->scope;
            reference(&E->scope);
            E->scope=function_scope;
            Object result=execute_ast(E, ((ASTSourcePointer*)f->source_pointer)->body, true);
            E->scope=previous_scope;
            dereference(E, &function_scope);
            return result;
        }
        default:
            RETURN_ERROR("CALL_ERROR", wrap_heap_object((HeapObject*)f), "Function type has incorrect type value of %i", f->ftype)
    }
}

bool is_arguments_count_correct(Executor* E, Function* function, int arguments_count, Object* error){
    int minimal_arguments=(int)function->arguments_count-function->optional_arguments_count-function->variadic;
    int max_arguments=function->arguments_count;
    if(arguments_count<minimal_arguments){
        NEW_ERROR(*error, "CALL_ERROR", wrap_heap_object((HeapObject*)function), 
        "Not enough arguments in function call, expected at least %i, given %i.", minimal_arguments, arguments_count);
        return false;
    } else if(!function->variadic && arguments_count>max_arguments) {
        NEW_ERROR(*error, "CALL_ERROR", wrap_heap_object((HeapObject*)function), 
        "Too many arguments in function call, expected %i, given %i.", max_arguments, arguments_count);
        return false;
    }
    return true;
}

// return error if arguments count is incorrect and proccess variadic functions, then call the Function using call_function_processed
Object call_function(Executor* E, Function* f, Object* arguments, int arguments_count){
     
    Object error;
    if(!is_arguments_count_correct(E, f, arguments_count, &error)){
        return error;
    }

    // native functions don't need their variadic arguments to be packed
    if((f->variadic||f->optional_arguments_count>0)&&f->ftype!=f_native){
        // make new arguments array
        int processed_arguments_count=f->arguments_count;
        int minimal_arguments=(int)f->arguments_count-f->optional_arguments_count-f->variadic;
        int normal_arguments_count=MAX(minimal_arguments, arguments_count);
        Object* processed_arguments=malloc(sizeof(Object)*processed_arguments_count);
        // copy non variadic arguments
        for(int i=0; i<normal_arguments_count; i++){
            processed_arguments[i]=arguments[i];
        }
        // add undefined arguments
        int undefined_arguments=processed_arguments_count-arguments_count;
        if(undefined_arguments>0){
            for(int i=0; i<f->optional_arguments_count; i++){
                processed_arguments[normal_arguments_count+i]=E->undefined_argument;
                reference(&processed_arguments[f->arguments_count+i]);
            }
        }
        if(f->variadic){
            // pack variadic arguments into a Table
            Object variadic_table;
            table_init(E, &variadic_table);
            int variadic_arguments_start=f->arguments_count-1;
            for(int i=variadic_arguments_start; i<arguments_count; i++){
                set(E, variadic_table, to_int(i-variadic_arguments_start), arguments[i]);
            }
            // append the variadic array to the end of processed arguments array
            processed_arguments[f->arguments_count-1]=variadic_table;
        }
        return call_function_processed(E, f, processed_arguments, processed_arguments_count);
    } else {
        return call_function_processed(E, f, arguments, arguments_count);
    }
}