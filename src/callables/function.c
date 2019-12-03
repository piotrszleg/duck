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
            }
            move_to_function(E, f);
            BytecodeProgram* bytecode_program=(BytecodeProgram*)f->source_pointer;
            if(bytecode_program->compiled!=NULL){
                Object result;
                bytecode_program->compiled(E, bytecode_program, &result);
                pop_return_point(E);
                return result;
            } else {
                return execute_bytecode(E);
            }
        case f_ast: {
            Object function_scope;
            table_init(E, &function_scope);
            if(f->enclosing_scope.type!=t_null){
                inherit_scope(E, function_scope, f->enclosing_scope);
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

// return error if arguments count is incorrect and proccess variadic functions, then call the Function using call_function_processed
Object call_function(Executor* E, Function* f, Object* arguments, int arguments_count){

    #define CALL_ERROR(message, ...) \
        RETURN_ERROR("CALL_ERROR", wrap_heap_object((HeapObject*)f),message, ##__VA_ARGS__)
    
    int arguments_count_difference=arguments_count-f->arguments_count+f->variadic;
    if(arguments_count_difference<0){
        CALL_ERROR("Not enough arguments in function call, expected at least %i, given %i.", f->arguments_count-f->variadic, arguments_count);
    } else if(!f->variadic && arguments_count_difference>0) {
        CALL_ERROR("Too many arguments in function call, expected %i, given %i.", f->arguments_count, arguments_count);
    }

    if(f->variadic&&f->ftype!=f_native){
        // make new arguments array
        int processed_arguments_count=f->arguments_count;
        Object* processed_arguments=malloc(sizeof(Object)*processed_arguments_count);
        // copy non variadic arguments
        for(int i=0; i<f->arguments_count-1; i++){
            processed_arguments[i]=arguments[i];
        }
        // pack variadic arguments into a Table
        Object variadic_table;
        table_init(E, &variadic_table);
        for(int i=arguments_count_difference-1; i>=0; i--){
            set(E, variadic_table, to_int(i), arguments[f->arguments_count-1+i]);
        }
        // append the variadic array to the end of processed arguments array
        processed_arguments[f->arguments_count-1]=variadic_table;

        return call_function_processed(E, f, processed_arguments, processed_arguments_count);
    } else {
        return call_function_processed(E, f, arguments, arguments_count);
    }
}