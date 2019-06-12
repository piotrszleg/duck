#include "ast_to_bytecode.h"

#define CODE_SIZE 32
#define CONSTANTS_SIZE 32
#define SUB_PROGRAMS_SIZE 4

int labels_count=0;

void ast_to_bytecode_recursive(expression* exp, BytecodeTranslation* translation, bool keep_scope);
BytecodeProgram closure_to_bytecode(function_declaration* d);

void repeat_information(BytecodeTranslation* translation){
    stream_push(&translation->information, &translation->last_information, sizeof(InstructionInformation));
}

void push_instruction(BytecodeTranslation* translation, InstructionType type, long value){
    Instruction instr={type, value};
    repeat_information(translation);
    stream_push(&translation->code, &instr, sizeof(Instruction));
}

char* stream_search_string(stream* s, const char* str){
    char* casted=(char*)s->data;
    if(s->position==0){
        return NULL;// there is nothing inside of the stream
    }
    for(int i=0; i<s->size; i++){
        char* stream_part=casted+i;// take part of the stream starting from i, we assume it's \0 terminated
        if(strcmp(stream_part, str)==0){
            return stream_part;
        }
    }
    return NULL;// string wasn't found inside of the stream
}

void push_string_load(BytecodeTranslation* translation, const char* string_constant){
    repeat_information(translation);
    char* search_result=stream_search_string(&translation->constants, string_constant);
    if(search_result){
        int relative_position=(search_result-((char*)translation->constants.data));
        push_instruction(translation, b_load_string, relative_position/sizeof(char));// use existing
    } else {
        int push_position=stream_push(&translation->constants, (const void*)string_constant, (strlen(string_constant)+1));
        push_instruction(translation, b_load_string, push_position);
    }
}

void push_number_load(BytecodeTranslation* translation, float number_constant){
    repeat_information(translation);
    long argument;
    memcpy (&argument, &number_constant, sizeof argument);
    push_instruction(translation, b_load_number, argument);
}

void stream_repeat_last(stream* s, unsigned repetitions, size_t element_size){
    void* last=(char*)s->position-element_size;
    for(int i=0; i<repetitions; i++){
        stream_push(s, last, element_size);
    }
}

InstructionInformation information_from_ast(expression* exp){
    InstructionInformation info;
    info.line=exp->line_number;
    info.column=exp->column_number;
    info.file=0;
    return info;
}

void bytecode_path_get(BytecodeTranslation* translation, path p){
    int lines_count=vector_total(&p.lines);
    for (int i = 0; i < lines_count; i++){
        expression* e= vector_get(&p.lines, i);
        if(e->type==e_name){
            push_string_load(translation, ((name*)e)->value);
        } else{
            ast_to_bytecode_recursive(e, translation, 0);
        }
        push_instruction(translation, i==0 ? b_get : b_table_get, 0);
    }
} 

void bytecode_path_set(BytecodeTranslation* translation, path p, bool used_in_closure){
    int lines_count=vector_total(&p.lines);
    for (int i = 0; i < lines_count; i++){
        expression* e= vector_get(&p.lines, i);
        if(e->type==e_name){
            push_string_load(translation, ((name*)e)->value);
        } else{
            ast_to_bytecode_recursive(e, translation, 0);
        }
        if(i==lines_count-1){
            // final isntruction is always set
            push_instruction(translation, i==0 ? b_set : b_table_set, used_in_closure);
        } else {
            push_instruction(translation, i==0 ? b_get : b_table_get, 0);
        }
    }
}

void ast_to_bytecode_recursive(expression* exp, BytecodeTranslation* translation, bool keep_scope){
    translation->last_information=information_from_ast(exp);

    switch(exp->type){
        case e_expression:
        case e_macro_declaration:
        case e_macro:
        case e_empty:
            push_instruction(translation, b_null, 0);
            break;
        case e_float_literal:
            push_number_load(translation, ((float_literal*)exp)->value);
            break;
        case e_int_literal:
            push_number_load(translation, ((int_literal*)exp)->value);
            break;
        case e_string_literal:
            push_string_load(translation, ((string_literal*)exp)->value);
            break;
        case e_table_literal:
        {
            table_literal* b=(table_literal*)exp;
            int lines_count=vector_total(&b->lines);
            push_instruction(translation, b_table_literal, 0);
            if(lines_count>0){
                int last_index=0;
                for (int i = 0; i < lines_count; i++){
                    expression* line=vector_get(&b->lines, i);
                    if(line->type==e_assignment){
                        assignment* assignment_line=((assignment*)line);
                        ast_to_bytecode_recursive(assignment_line->right, translation, 1);
                        push_string_load(translation, table_literal_extract_key(assignment_line));// assuming that assignment_line inside of Table always has one item
                        push_instruction(translation, b_table_set_keep, 0);
                    } else {
                        // if the expression isn't assignment use last index as a key
                        // arr=[5, 4] => arr[0=5, 1=4]
                        ast_to_bytecode_recursive(line, translation, 1);
                        push_number_load(translation, (float)last_index++);
                        push_instruction(translation, b_table_set_keep, 0);
                    }
                }
            }
            break;
        }
        case e_block:
        {
            block* b=(block*)exp;

            if(!keep_scope){
                push_instruction(translation, b_get_scope, 0);
                push_instruction(translation, b_new_scope, 0);
            }

            int lines_count=vector_total(&b->lines);
            for (int i = 0; i < lines_count; i++){
                ast_to_bytecode_recursive(vector_get(&b->lines, i), translation, false);
                if(i!=lines_count-1){// result of the last line isn't discarded
                    push_instruction(translation, b_discard, 0);
                }
            }
            if(!keep_scope){
                // the result of evaluating the last line is now on top
                // so the scope object needs to be pushed to the top instead
                push_instruction(translation, b_push_to_top, 1);
                push_instruction(translation, b_set_scope, 0);
            }

            break;
        }
        case e_name:
        {
            name* n=(name*)exp;
            push_string_load(translation, n->value);
            push_instruction(translation, b_get, 0);
            break;
        }
        case e_assignment:
        {
            assignment* a=(assignment*)exp;
            ast_to_bytecode_recursive(a->right, translation, false);
            bytecode_path_set(translation, *a->left, a->used_in_closure);
            break;
        }
        case e_binary:
        {
            binary* u=(binary*)exp;

            ast_to_bytecode_recursive(u->right, translation, false);
            ast_to_bytecode_recursive(u->left, translation, false);
            push_string_load(translation, u->op);
            push_instruction(translation, b_binary, 0);
            break;
        }
        case e_prefix:
        {
            prefix* p=(prefix*)exp;
            ast_to_bytecode_recursive(p->right, translation, false);

            push_string_load(translation, p->op);
            push_instruction(translation, b_prefix, 0);
            break;
        }
        case e_conditional:
        {
            conditional* c=(conditional*)exp;
            int conditional_end=labels_count++;
            int on_false=labels_count++;

            ast_to_bytecode_recursive(c->condition, translation, false);
            push_instruction(translation, b_jump_not, on_false);// if condition is false jump to on_false label

            ast_to_bytecode_recursive(c->ontrue, translation, false);
            push_instruction(translation, b_jump, conditional_end);// jump over the on_false block to the end of conditional

            push_instruction(translation, b_label, on_false);
            ast_to_bytecode_recursive(c->onfalse, translation, false);

            push_instruction(translation, b_label, conditional_end);

            break;
        }
        case e_function_declaration:
        {
            function_declaration* d=(function_declaration*)exp;
            int arguments_count=vector_total(&d->arguments);

            push_number_load(translation, (float)d->variadic);
            push_number_load(translation, (float)arguments_count);
            BytecodeProgram prog=closure_to_bytecode(d);
            stream_push(&translation->sub_programs, &prog, sizeof(BytecodeProgram));
            int sub_program_index=(translation->sub_programs.position/sizeof(BytecodeProgram))-1;
            push_instruction(translation, b_function, sub_program_index);
            break;
        }
        case e_function_call:
        {
            function_call* c=(function_call*)exp;
            int lines_count=vector_total(&c->arguments->lines);
            for (int i = 0; i < lines_count; i++){
                ast_to_bytecode_recursive(vector_get(&c->arguments->lines, i), translation, false);
            }
            ast_to_bytecode_recursive(c->called, translation, false);
            push_instruction(translation, b_call, lines_count);
            break;
        }
        case e_parentheses:
        {
            ast_to_bytecode_recursive(((parentheses*)exp)->value, translation, false);
            break;
        }
        case e_function_return:
        {
            function_return* r=(function_return*)exp;
            ast_to_bytecode_recursive((expression*)r->value, translation, false);
            push_instruction(translation, b_return, 0);
            break;
        }
        case e_path:
        {
            bytecode_path_get(translation, *((path*)exp));
            break;
        }
        default:
        {
            THROW_ERROR(WRONG_ARGUMENT_TYPE, "Uncatched expression Instruction type: %i\n", exp->type);
        }
    }
}

void bytecode_translation_init(BytecodeTranslation* translation){
    stream_init(&translation->constants, CONSTANTS_SIZE);
    stream_init(&translation->code, CODE_SIZE);
    stream_init(&translation->information, CODE_SIZE);
    stream_init(&translation->sub_programs, SUB_PROGRAMS_SIZE);
}

BytecodeProgram translation_to_bytecode(BytecodeTranslation* translation){
    BytecodeProgram prog;
    stream_truncate(&translation->code);
    stream_truncate(&translation->information);
    stream_truncate(&translation->constants);
    stream_truncate(&translation->sub_programs);
    prog.code=stream_get_data(&translation->code);
    prog.constants=stream_get_data(&translation->constants);
    prog.constants_size=translation->constants.position;
    prog.information=stream_get_data(&translation->information);
    prog.sub_programs=stream_get_data(&translation->sub_programs);
    prog.sub_programs_count=translation->sub_programs.position/sizeof(BytecodeProgram);
    return prog;
}

void finish_translation(BytecodeTranslation* translation) {
    push_instruction(translation, b_end, 0);
    repeat_information(translation);
}

BytecodeProgram closure_to_bytecode(function_declaration* d){
    BytecodeTranslation translation;
    bytecode_translation_init(&translation);

    int arguments_count=vector_total(&d->arguments);
    for (int i = 0; i < arguments_count; i++){
        push_string_load(&translation, ((name*)vector_get(&d->arguments, arguments_count-1-i))->value);
        push_instruction(&translation, b_set, 0);
        push_instruction(&translation, b_discard, 0);
    }

    ast_to_bytecode_recursive(d->body, &translation, true);
    finish_translation(&translation);

    return translation_to_bytecode(&translation);
}

BytecodeProgram ast_to_bytecode(expression* exp, bool keep_scope){
    BytecodeTranslation translation;
    bytecode_translation_init(&translation);

    ast_to_bytecode_recursive(exp, &translation, keep_scope);
    finish_translation(&translation);

    return translation_to_bytecode(&translation);
}