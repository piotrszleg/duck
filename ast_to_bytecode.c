#include "ast_to_bytecode.h"

#define CODE_SIZE 50
#define CONSTANTS_SIZE 50

int labels_count=0;

void ast_to_bytecode_recursive(expression* exp, stream* code, stream* constants, int keep_scope);

void push_instruction(stream* code, instruction_type type, long value){
    instruction instr={type, value};
    stream_push(code, &instr, sizeof(instruction));
}

char* stream_search_string(stream* s, const char* str){
    char* casted=(char*)s->data;
    int casted_size=s->size/sizeof(char*);
    for(int i=0; i<casted_size; i++){
        char* stream_part=casted+i;
        if(strcmp(stream_part, str)==0){
            return stream_part;
        }
    }
    return NULL;
}

void push_string_load(stream* code, stream* constants, const char* string_constant){
    char* search_result=stream_search_string(constants, string_constant);
    if(search_result){
        int relative_position=(search_result-((char*)constants->data));
        push_instruction(code, b_load_string, relative_position/sizeof(char));// use existing
    } else {
        int push_position=stream_push(constants, (const void*)string_constant, (strlen(string_constant)+1));
        push_instruction(code, b_load_string, push_position);
    }
}

void push_number_load(stream* code, float number_constant){
    long argument;
    memcpy (&argument, &number_constant, sizeof argument);
    push_instruction(code, b_load_number, argument);
}

void bytecode_path_get(stream* code, stream* constants, path p){
    int lines_count=vector_total(&p.lines);
    for (int i = 0; i < lines_count; i++){
        expression* e= vector_get(&p.lines, i);
        if(e->type==_name){
            push_string_load(code, constants, ((name*)e)->value);
        } else{
            ast_to_bytecode_recursive(e, code, constants, 0);
        }
        instruction get={b_get, i>0};
        stream_push(code, &get, sizeof(instruction));
    }
} 

void bytecode_path_set(stream* code, stream* constants, path p){
    int lines_count=vector_total(&p.lines);
    for (int i = 0; i < lines_count; i++){
        expression* e= vector_get(&p.lines, i);
        if(e->type==_name){
            push_string_load(code, constants, ((name*)e)->value);
        } else{
            ast_to_bytecode_recursive(e, code, constants, 0);
        }
        if(i==lines_count-1){
            instruction set={b_set, i>0};
            stream_push(code, &set, sizeof(instruction));
        } else {
            instruction get={b_get, i>0};
            stream_push(code, &get, sizeof(instruction));
        }
    }
}


void ast_to_bytecode_recursive(expression* exp, stream* code, stream* constants, int keep_scope){
    switch(exp->type){
        case _empty:
            push_instruction(code, b_null, 0);
            break;
        case _literal:
        {
            literal* l=(literal*)exp;
            switch(l->ltype){
                case _int:
                {
                    // all numbers in duck are floats, so int must be converted to float
                    push_number_load(code, (float)l->ival);
                    break;
                }
                case _float:
                {
                    push_number_load(code, l->fval);
                    break;
                }
                case _string:
                {
                    push_string_load(code, constants, l->sval);
                    break;
                }
            }
            break;
        }
        case _table_literal:
        {
            block* b=(block*)exp;
            
            push_instruction(code, b_get_scope, 0);
            push_instruction(code, b_table_literal, 0);
            push_instruction(code, b_set_scope, 0);

            int last_index=0;
            for (int i = 0; i < vector_total(&b->lines); i++){
                expression* line=vector_get(&b->lines, i);
                ast_to_bytecode_recursive(line, code, constants, 1);
                if(line->type!=_assignment){
                    // if the expression isn't assignment use last index as a key
                    // arr=[5, 4] => arr[0=5, 1=2]
                    push_number_load(code, (float)last_index++);
                    push_instruction(code, b_set, 0);
                }
                push_instruction(code, b_discard, 0);
            }

            push_instruction(code, b_get_scope, 0);
            push_instruction(code, b_swap, 0);
            push_instruction(code, b_set_scope, 0);

            break;
        }
        case _block:
        {
            block* b=(block*)exp;
            int lines_count=vector_total(&b->lines);
            for (int i = 0; i < lines_count; i++){
                ast_to_bytecode_recursive(vector_get(&b->lines, i), code, constants, 1);
                if(i!=lines_count-1){// leave only the last line on the stack
                    push_instruction(code, b_discard, 0);
                }
            }
            break;
        }
        case _name:
        {
            name* n=(name*)exp;
            push_string_load(code, constants, n->value);
            instruction get={b_get, 0};
            stream_push(code, &get, sizeof(instruction));
            break;
        }
        case _assignment:
        {
            assignment* a=(assignment*)exp;
            ast_to_bytecode_recursive(a->right, code, constants, keep_scope);
            bytecode_path_set(code, constants, *a->left);
            break;
        }
        case _unary:
        {
            unary* u=(unary*)exp;

            ast_to_bytecode_recursive(u->right, code, constants, keep_scope);
            ast_to_bytecode_recursive(u->left, code, constants, keep_scope);
            

            push_string_load(code, constants, u->op);
            push_instruction(code, b_unary, 0);
            break;
        }
        case _prefix:
        {
            prefix* p=(prefix*)exp;
            ast_to_bytecode_recursive(p->right, code, constants, keep_scope);

            push_string_load(code, constants, p->op);
            push_instruction(code, b_prefix, 0);
            break;
        }
        case _conditional:
        {
            conditional* c=(conditional*)exp;
            int conditional_end=labels_count++;
            int on_false=labels_count++;

            ast_to_bytecode_recursive(c->condition, code, constants, keep_scope);
            push_instruction(code, b_jump_not, on_false);// if condition is false jump to on_false label

            ast_to_bytecode_recursive(c->ontrue, code, constants, keep_scope);
            push_instruction(code, b_jump, conditional_end);// jump over the on_false block to the end of conditional

            push_instruction(code, b_label, on_false);
            ast_to_bytecode_recursive(c->onfalse, code, constants, keep_scope);

            push_instruction(code, b_label, conditional_end);

            break;
        }
        case _function_declaration:
        {
            function_declaration* d=(function_declaration*)exp;
            int arguments_count=vector_total(d->arguments);

            int function_end=labels_count++;
            int function_beginning=labels_count++;

            push_instruction(code, b_jump, function_end);// jump over the function
            push_instruction(code, b_label, function_beginning);// this is where function object will point
            
            // arguments need to be in reverse because of the way stack works
            for (int i = 0; i < arguments_count; i++){
                push_string_load(code, constants, ((name*)vector_get(d->arguments, arguments_count-1-i))->value);
                push_instruction(code, b_set, 0);
                push_instruction(code, b_discard, 0);
            }

            ast_to_bytecode_recursive((expression*)d->body, code, constants, keep_scope);

            push_instruction(code, b_return, 0);
            push_instruction(code, b_label, function_end);

            /*for (int i = 0; i < arguments_count; i++){
                push_string_load(code, constants, ((name*)vector_get(d->arguments, i))->value);
            }*/
            push_number_load(code, (float)arguments_count);
            push_instruction(code, b_function, function_beginning);
            break;
        }
        case _function_call:
        {
            function_call* c=(function_call*)exp;
            int lines_count=vector_total(&c->arguments->lines);
            for (int i = 0; i < lines_count; i++){
                ast_to_bytecode_recursive(vector_get(&c->arguments->lines, i), code, constants, keep_scope);
            }
            bytecode_path_get(code, constants, *c->function_path);
            push_instruction(code, b_call, lines_count);
            break;
        }
        case _function_return:
        {
            function_return* r=(function_return*)exp;
            ast_to_bytecode_recursive((expression*)r->value, code, constants, keep_scope);
            push_instruction(code, b_return, 0);
            break;
        }
        case _path:
        {
            bytecode_path_get(code, constants, *((path*)exp));
            break;
        }
        default:
        {
            ERROR(WRONG_ARGUMENT_TYPE, "Uncatched expression instruction type: %i\n", exp->type);
        }
    }
}

bytecode_program ast_to_bytecode(expression* exp, int keep_scope){
    stream constants;
    stream code;
    init_stream(&constants, CONSTANTS_SIZE);
    init_stream(&code, CODE_SIZE);

    ast_to_bytecode_recursive(exp, &code, &constants, keep_scope);
    instruction instr={b_end, 0};
    stream_push(&code, &instr, sizeof(instruction));

    bytecode_program prog;
    stream_truncate(&code);
    stream_truncate(&constants);
    prog.code=code.data;
    prog.constants=constants.data;
    return prog;
}