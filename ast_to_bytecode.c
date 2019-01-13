#include "ast_to_bytecode.h"

#define CODE_SIZE 32
#define CONSTANTS_SIZE 32
#define SUB_PROGRAMS_SIZE 4

int labels_count=0;

void ast_to_bytecode_recursive(expression* exp, bytecode_translation* translation, int keep_scope);
bytecode_program closure_to_bytecode(function_declaration* d);

void push_instruction(stream* code, instruction_type type, long value){
    instruction instr={type, value};
    stream_push(code, &instr, sizeof(instruction));
}

char* stream_search_string(stream* s, const char* str){
    char* casted=(char*)s->data;
    int casted_size=s->size;
    for(int i=0; i<casted_size; i++){
        char* stream_part=casted+i;
        if(strcmp(stream_part, str)==0){
            return stream_part;
        }
    }
    return NULL;
}

void push_string_load(bytecode_translation* translation, const char* string_constant){
    char* search_result=stream_search_string(&translation->constants, string_constant);
    if(search_result){
        int relative_position=(search_result-((char*)translation->constants.data));
        push_instruction(&translation->code, b_load_string, relative_position/sizeof(char));// use existing
    } else {
        int push_position=stream_push(&translation->constants, (const void*)string_constant, (strlen(string_constant)+1));
        push_instruction(&translation->code, b_load_string, push_position);
    }
}

void push_number_load(stream* code, float number_constant){
    long argument;
    memcpy (&argument, &number_constant, sizeof argument);
    push_instruction(code, b_load_number, argument);
}

void stream_repeat_last(stream* s, unsigned repetitions, size_t element_size){
    void* last=(char*)s->position-element_size;
    for(int i=0; i<repetitions; i++){
        stream_push(s, last, element_size);
    }
}

instruction_information information_from_ast(expression* exp){
    instruction_information info;
    info.line=exp->line_number;
    info.column=exp->column_number;
    info.file=0;
    return info;
}

void bytecode_path_get(bytecode_translation* translation, path p){
    int lines_count=vector_total(&p.lines);
    for (int i = 0; i < lines_count; i++){
        expression* e= vector_get(&p.lines, i);
        if(e->type==e_name){
            instruction_information info=information_from_ast(e);
            stream_push(&translation->information, &info, sizeof(info));
            push_string_load(translation, ((name*)e)->value);
        } else{
            ast_to_bytecode_recursive(e, translation, 0);
        }
        instruction_information info=information_from_ast((expression*)&p);
        stream_push(&translation->information, &info, sizeof(info));
        instruction get={b_get, i>0};
        stream_push(&translation->code, &get, sizeof(instruction));
    }
} 

void bytecode_path_set(bytecode_translation* translation, path p){
    int lines_count=vector_total(&p.lines);
    for (int i = 0; i < lines_count; i++){
        expression* e= vector_get(&p.lines, i);
        if(e->type==e_name){
            instruction_information info=information_from_ast(e);
            stream_push(&translation->information, &info, sizeof(info));
            push_string_load(translation, ((name*)e)->value);
        } else{
            ast_to_bytecode_recursive(e, translation, 0);
        }
        if(i==lines_count-1){
            instruction set={b_set, i>0};
            stream_push(&translation->code, &set, sizeof(instruction));
        } else {
            instruction get={b_get, i>0};
            stream_push(&translation->code, &get, sizeof(instruction));
        }
        instruction_information info=information_from_ast((expression*)&p);
        stream_push(&translation->information, &info, sizeof(info));
    }
}

void ast_to_bytecode_recursive(expression* exp, bytecode_translation* translation, int keep_scope){
    instruction_information info=information_from_ast(exp);

    stream* code=&translation->code;

    #define DUPLICATE_INFO(repetitions) stream_push(&translation->information, &info, sizeof(info));//stream_repeat_last(information, repetitions, sizeof(instruction_information));
    #define PUSH_INFO stream_push(&translation->information, &info, sizeof(info));

    switch(exp->type){
        case e_empty:
            PUSH_INFO
            push_instruction(&translation->code, b_null, 0);
            break;
        case e_literal:
        {
            PUSH_INFO
            literal* l=(literal*)exp;
            switch(l->ltype){
                case l_int:
                    // all numbers in duck are floats, so int must be converted to float
                    push_number_load(code, (float)l->ival);
                    break;
                case l_float:
                    push_number_load(code, l->fval);
                    break;
                case l_string:
                    push_string_load(translation, l->sval);
                    break;
            }
            break;
        }
        case e_table_literal:
        {
            block* b=(block*)exp;
            
            PUSH_INFO
            DUPLICATE_INFO(2)
            push_instruction(code, b_get_scope, 0);
            push_instruction(code, b_table_literal, 0);
            push_instruction(code, b_set_scope, 0);

            int last_index=0;
            for (int i = 0; i < vector_total(&b->lines); i++){
                expression* line=vector_get(&b->lines, i);
                ast_to_bytecode_recursive(line, translation, 1);
                if(line->type!=e_assignment){
                    // if the expression isn't assignment use last index as a key
                    // arr=[5, 4] => arr[0=5, 1=4]
                    PUSH_INFO
                    PUSH_INFO
                    push_number_load(code, (float)last_index++);
                    push_instruction(code, b_set, 0);
                }
                DUPLICATE_INFO(1)
                push_instruction(code, b_discard, 0);
            }

            PUSH_INFO
            DUPLICATE_INFO(2)
            push_instruction(code, b_get_scope, 0);
            push_instruction(code, b_swap, 1);
            push_instruction(code, b_set_scope, 0);

            break;
        }
        case e_block:
        {
            block* b=(block*)exp;
            int lines_count=vector_total(&b->lines);
            for (int i = 0; i < lines_count; i++){
                ast_to_bytecode_recursive(vector_get(&b->lines, i), translation, 1);
                if(i!=lines_count-1){// leave only the last line on the stack
                    push_instruction(code, b_discard, 0);
                    PUSH_INFO
                }
            }
            break;
        }
        case e_name:
        {
            name* n=(name*)exp;
            push_string_load(translation, n->value);
            PUSH_INFO
            push_instruction(code, b_get, 0);
            PUSH_INFO
            break;
        }
        case e_assignment:
        {
            assignment* a=(assignment*)exp;
            ast_to_bytecode_recursive(a->right, translation, keep_scope);
            bytecode_path_set(translation, *a->left);
            PUSH_INFO
            break;
        }
        case e_unary:
        {
            unary* u=(unary*)exp;

            ast_to_bytecode_recursive(u->right, translation, keep_scope);
            ast_to_bytecode_recursive(u->left, translation, keep_scope);
            push_string_load(translation, u->op);
            PUSH_INFO
            push_instruction(code, b_unary, 0);
            PUSH_INFO
            break;
        }
        case e_prefix:
        {
            prefix* p=(prefix*)exp;
            ast_to_bytecode_recursive(p->right, translation, keep_scope);

            PUSH_INFO
            push_string_load(translation, p->op);
            PUSH_INFO
            push_instruction(code, b_prefix, 0);
            break;
        }
        case e_conditional:
        {
            conditional* c=(conditional*)exp;
            int conditional_end=labels_count++;
            int on_false=labels_count++;

            ast_to_bytecode_recursive(c->condition, translation, keep_scope);
            push_instruction(code, b_jump_not, on_false);// if condition is false jump to on_false label
            DUPLICATE_INFO(1)

            ast_to_bytecode_recursive(c->ontrue, translation, keep_scope);
            push_instruction(code, b_jump, conditional_end);// jump over the on_false block to the end of conditional
            DUPLICATE_INFO(1)

            push_instruction(code, b_label, on_false);
            ast_to_bytecode_recursive(c->onfalse, translation, keep_scope);
            DUPLICATE_INFO(1)

            push_instruction(code, b_label, conditional_end);
            DUPLICATE_INFO(1)

            break;
        }
        case e_function_declaration:
        {
            function_declaration* d=(function_declaration*)exp;
            int arguments_count=vector_total(d->arguments);

            PUSH_INFO
            PUSH_INFO
            push_number_load(code, (float)arguments_count);
            bytecode_program prog=closure_to_bytecode(d);
            stream_push(&translation->sub_programs, &prog, sizeof(bytecode_program));
            int sub_program_index=(translation->sub_programs.position/sizeof(bytecode_program))-1;
            push_instruction(code, b_function, sub_program_index);
            break;
        }
        case e_function_call:
        {
            function_call* c=(function_call*)exp;
            int lines_count=vector_total(&c->arguments->lines);
            for (int i = 0; i < lines_count; i++){
                ast_to_bytecode_recursive(vector_get(&c->arguments->lines, i), translation, keep_scope);
            }
            PUSH_INFO
            bytecode_path_get(translation, *c->function_path);
            PUSH_INFO
            push_instruction(code, b_call, lines_count);
            break;
        }
        case e_function_return:
        {
            function_return* r=(function_return*)exp;
            ast_to_bytecode_recursive((expression*)r->value, translation, keep_scope);
            PUSH_INFO
            push_instruction(code, b_return, 0);
            break;
        }
        case e_path:
        {
            bytecode_path_get(translation, *((path*)exp));
            break;
        }
        default:
        {
            ERROR(WRONG_ARGUMENT_TYPE, "Uncatched expression instruction type: %i\n", exp->type);
        }
    }
    #undef DUPLICATE_INFO
    #undef PUSH_INFO
}

void bytecode_translation_init(bytecode_translation* translation){
    init_stream(&translation->constants, CONSTANTS_SIZE);
    init_stream(&translation->code, CODE_SIZE);
    init_stream(&translation->information, CODE_SIZE);
    init_stream(&translation->sub_programs, SUB_PROGRAMS_SIZE);
}

bytecode_program translation_to_bytecode(bytecode_translation* translation){
    bytecode_program prog;
    stream_truncate(&translation->code);
    stream_truncate(&translation->information);
    stream_truncate(&translation->constants);
    stream_truncate(&translation->sub_programs);
    prog.code=translation->code.data;
    prog.constants=translation->constants.data;
    prog.information=translation->information.data;
    prog.sub_programs=translation->sub_programs.data;
    prog.sub_programs_count=translation->sub_programs.position/sizeof(bytecode_program);
    return prog;
}

bytecode_program closure_to_bytecode(function_declaration* d){
    bytecode_translation translation;
    bytecode_translation_init(&translation);

    instruction_information info=information_from_ast((expression*)d);
    #define PUSH_INFO stream_push(&translation.information, &info, sizeof(info));

    int arguments_count=vector_total(d->arguments);
    for (int i = 0; i < arguments_count; i++){
        PUSH_INFO
        PUSH_INFO
        PUSH_INFO
        push_string_load(&translation, ((name*)vector_get(d->arguments, arguments_count-1-i))->value);
        push_instruction(&translation.code, b_set, 0);
        push_instruction(&translation.code, b_discard, 0);
    }

    ast_to_bytecode_recursive(d->body, &translation, true);
    instruction instr={b_end, 0};
    stream_push(&translation.code, &instr, sizeof(instruction));

    return translation_to_bytecode(&translation);
    #undef PUSH_INFO
}

bytecode_program ast_to_bytecode(expression* exp, int keep_scope){
    bytecode_translation translation;
    bytecode_translation_init(&translation);

    ast_to_bytecode_recursive(exp, &translation, keep_scope);
    instruction instr={b_end, 0};
    stream_push(&translation.code, &instr, sizeof(instruction));

    return translation_to_bytecode(&translation);
}