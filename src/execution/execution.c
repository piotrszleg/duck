#include "execution.h"

Object evaluate(Executor* E, Expression* ast, Object scope, const char* file_name, bool delete_ast){
    if(ast==NULL){
        return null_const;// there was an error while parsing
    }
    create_return_point(E, true);
    reference(&scope);
    E->scope=scope;
    Object macro_evaluation_error=execute_macros(E, &ast);
    if(macro_evaluation_error.type!=t_null){
        delete_expression(ast);
        return macro_evaluation_error;
    }
    if(E->options.optimise_ast){
        optimise_ast(E, &ast);
    }
    if(E->options.print_ast){
        USING_STRING(stringify_expression(ast, 0),
            printf("Abstract Syntax Tree:\n%s\n", str));
    }

    if(!E->options.disable_ast_execution){
        Object execution_result=execute_ast(E, ast, true);
        if(delete_ast){
            delete_expression(ast);
        }
        return execution_result;
    } else if(!E->options.disable_bytecode) {
        BytecodeProgram* bytecode_program=ast_to_bytecode(ast, true);
        if(bytecode_program!=NULL){
            bytecode_program->source_file_name=strdup(file_name);
            if(delete_ast){
                delete_expression(ast);// at this point ast is useless and only wastes memory
            }
            if(E->options.optimise_bytecode){
                optimise_bytecode(E, bytecode_program, E->options.print_bytecode_optimisations);
            }
            bytecode_program_init(E, bytecode_program);
            if(E->options.print_bytecode){
                print_bytecode_program(bytecode_program);
            }
            E->bytecode_environment.pointer=0;
            E->bytecode_environment.executed_program=bytecode_program;
            // the end instruction will dereference these later
            heap_object_reference((HeapObject*)E->bytecode_environment.executed_program);
            return execute_bytecode(E);
        } else {
            RETURN_ERROR("EVALUATION_ERROR", null_const, 
                "Bytecode generation failed and ast execution is disabled.")
        }
    } else {
        RETURN_ERROR("EVALUATION_ERROR", null_const, 
            "Both bytecode and ast execution are disabled.")
    }
}

Object evaluate_string(Executor* E, const char* s, Object scope){
    Expression* parsing_result=parse_string(s);
    E->file="string";
    return evaluate(E, parsing_result, scope, "string", true);
}

Object evaluate_file(Executor* E, const char* file_name, Object scope){
    Expression* parsing_result=parse_file(file_name);
    E->file=file_name;
    return evaluate(E, parsing_result, scope, file_name, true);
}

static Object arguments_to_table(Executor* E, const char* file_name, char** arguments){
    Object arguments_table;
    table_init(E, &arguments_table);
    table_set(E, arguments_table.tp, to_int(0), to_string(file_name));
    int i=0;
    if(arguments!=NULL){
        for(; arguments[i]!=NULL; i++){
            table_set(E, arguments_table.tp, to_int(i+1), to_string(arguments[i]));
        }
    }
    table_set(E, arguments_table.tp, to_string("count"), to_int(i+1));
    return arguments_table;
}

void execute_file(Executor* E, const char* file_name){
    Object global_scope;
    table_init(E, &global_scope);
    Object patching_table;
    table_init(E, &patching_table);
    table_set(E, global_scope.tp, to_string("patching_table"), patching_table);
    reference(&global_scope);
    table_set(E, global_scope.tp, to_string("arguments"), arguments_to_table(E, file_name, E->options.script_arguments));
    if(E->options.include_builtins){
        inherit_scope(E, global_scope, builtins_table(E));
    }
    Object execution_result=evaluate_file(E, file_name, global_scope);
    USING_STRING(stringify(E, execution_result), 
        printf("The script \"%s\" has exited with result:\n%s\n", file_name, str));
    dereference(E, &execution_result);
    dereference(E, &global_scope);
    E->scope=null_const;
}