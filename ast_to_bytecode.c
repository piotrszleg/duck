#include "ast_to_bytecode.h"

#define CODE_SIZE 32
#define CONSTANTS_SIZE 32
#define SUB_PROGRAMS_SIZE 4
#define UPVALUES_SIZE 8

int labels_count=0;

void ast_to_bytecode_recursive(expression* exp, BytecodeTranslation* translation, bool keep_scope);
BytecodeProgram closure_to_bytecode(function_declaration* d);

void repeat_information(BytecodeTranslation* translation){
    stream_push(&translation->information, &translation->last_information, sizeof(InstructionInformation));
}

void push_uint_instruction(BytecodeTranslation* translation, InstructionType type, unsigned int value){
    Instruction instr={type};
    instr.uint_argument=value;
    repeat_information(translation);
    stream_push(&translation->code, &instr, sizeof(Instruction));
}

void push_bool_instruction(BytecodeTranslation* translation, InstructionType type, bool value){
    Instruction instr={type};
    instr.bool_argument=value;
    repeat_information(translation);
    stream_push(&translation->code, &instr, sizeof(Instruction));
}

void push_instruction(BytecodeTranslation* translation, InstructionType type){
    Instruction instr={type};
    repeat_information(translation);
    stream_push(&translation->code, &instr, sizeof(Instruction));
}

char* stream_search_string(stream* s, const char* str){
    char* casted=(char*)s->data;
    if(s->position==0){
        return NULL;// there is nothing inside of the stream
    }
    for(int i=0; i<s->position; i++){
        char* stream_part=casted+i;// take part of the stream starting from i, we assume it's \0 terminated
        if(strcmp(stream_part, str)==0){
            return stream_part;
        }
    }
    return NULL;// string wasn't found inside of the stream
}

unsigned push_string_constant(BytecodeTranslation* translation, const char* string_constant) {
    char* search_result=stream_search_string(&translation->constants, string_constant);
    if(search_result!=NULL){
        int relative_position=(search_result-((char*)translation->constants.data));
        return (unsigned)(relative_position/sizeof(char));// reuse existing constant
    } else {
        unsigned int push_position=(unsigned int)stream_push(&translation->constants, (const void*)string_constant, (strlen(string_constant)+1));
        return push_position;
    }
}

unsigned push_string_load(BytecodeTranslation* translation, const char* string_constant){
    repeat_information(translation);
    unsigned push_position= push_string_constant(translation, string_constant);
    push_uint_instruction(translation, b_load_string, push_position);
    return push_position;
}

void push_float_load(BytecodeTranslation* translation, float number_constant){
    repeat_information(translation);
    Instruction instr={b_load_float};
    instr.float_argument=number_constant;
    stream_push(&translation->code, &instr, sizeof(Instruction));
}

void push_int_load(BytecodeTranslation* translation, int number_constant){
    repeat_information(translation);
    Instruction instr={b_load_int};
    instr.int_argument=number_constant;
    stream_push(&translation->code, &instr, sizeof(Instruction));
}

void stream_repeat_last(stream* s, unsigned repetitions, size_t element_size){
    void* last=(char*)s->position-element_size;
    for(int i=0; i<repetitions; i++){
        stream_push(s, last, element_size);
    }
}

InstructionInformation information_from_ast(BytecodeTranslation* translation, expression* exp){
    InstructionInformation info;
    info.line=exp->line_number;
    info.column=exp->column_number;
    info.file=0;
    info.comment=-1;
    
    return info;
}

bool is_upvalue(BytecodeTranslation* translation, char* key){
    Instruction* casted=(Instruction*)translation->code.data;
    for(int i=1; i<translation->code.position/sizeof(Instruction); i++){
        if(casted[i].type==b_set){
            Instruction previous=casted[i-1];
            if(previous.type==b_load_string){
                char* set_key=(char*)translation->constants.data+previous.uint_argument;
                if(strcmp(key, set_key)==0){
                    return false;
                }
            }
        }
    }
    return true;
}

void bytecode_path_get(BytecodeTranslation* translation, path p){
    expression* first_line= pointers_vector_get(&p.lines, 0);
    if(first_line->type!=e_name){
        USING_STRING(stringify_expression(first_line, 0),
            THROW_ERROR(BYTECODE_ERROR, "Variable name must be of type name. Caused by %s", str))
        return;
    }
    char* variable_name=((name*)first_line)->value;
    unsigned push_position=push_string_load(translation, variable_name);
    push_instruction(translation, b_get);
    if(is_upvalue(translation, variable_name)){
        stream_push(&translation->upvalues, &push_position, sizeof(unsigned));
    }
    int lines_count=vector_count(&p.lines);
    for (int i = 1; i < lines_count; i++){
        expression* e= pointers_vector_get(&p.lines, i);
        if(e->type==e_name){
            push_string_load(translation, ((name*)e)->value);
        } else{
            ast_to_bytecode_recursive(e, translation, 0);
        }
        push_instruction(translation, b_table_get);
    }
}

void table_literal_key(BytecodeTranslation* translation, path p) {
    if(vector_count(&p.lines)!=1){
        THROW_ERROR(WRONG_ARGUMENT_TYPE, "Number of lines in table literal key should be one.\n");
    } else {
        expression* e=pointers_vector_get(&p.lines, 0);
        if(e->type==e_name){
            push_string_load(translation, ((name*)e)->value);
        } else{
            ast_to_bytecode_recursive(e, translation, 0);
        }
    }
}

void bytecode_path_set(BytecodeTranslation* translation, path p, bool used_in_closure){
    int lines_count=vector_count(&p.lines);
    for (int i = 0; i < lines_count; i++){
        expression* e= pointers_vector_get(&p.lines, i);
        if(e->type==e_name){
            push_string_load(translation, ((name*)e)->value);
        } else{
            ast_to_bytecode_recursive(e, translation, 0);
        }
        if(i==lines_count-1){
            // final isntruction is always set
            push_bool_instruction(translation, i==0 ? b_set : b_table_set, used_in_closure);
        } else {
            push_instruction(translation, i==0 ? b_get : b_table_get);
        }
    }
}

void ast_to_bytecode_recursive(expression* exp, BytecodeTranslation* translation, bool keep_scope){
    translation->last_information=information_from_ast(translation, exp);

    switch(exp->type){
        case e_expression:
        case e_macro_declaration:
        case e_macro:
        case e_empty:
            push_uint_instruction(translation, b_null, 0);
            break;
        case e_float_literal:
            push_float_load(translation, ((float_literal*)exp)->value);
            break;
        case e_int_literal:
            push_int_load(translation, ((int_literal*)exp)->value);
            break;
        case e_string_literal:
            push_string_load(translation, ((string_literal*)exp)->value);
            break;
        case e_table_literal:
        {
            table_literal* b=(table_literal*)exp;
            int lines_count=vector_count(&b->lines);
            push_uint_instruction(translation, b_table_literal, 0);
            if(lines_count>0){
                int last_index=0;
                for (int i = 0; i < lines_count; i++){
                    expression* line=pointers_vector_get(&b->lines, i);
                    if(line->type==e_assignment){
                        assignment* assignment_line=((assignment*)line);
                        ast_to_bytecode_recursive(assignment_line->right, translation, true);
                        table_literal_key(translation, *assignment_line->left);// assuming that assignment_line inside of Table always has one item
                        push_instruction(translation, b_table_set_keep);
                    } else {
                        // if the expression isn't assignment use last index as the key
                        // arr=[5, 4] => arr[0=5, 1=4]
                        ast_to_bytecode_recursive(line, translation, 1);
                        push_int_load(translation, (float)last_index++);
                        push_instruction(translation, b_table_set_keep);
                    }
                }
            }
            break;
        }
        case e_block:
        {
            block* b=(block*)exp;

            if(!keep_scope){
                push_instruction(translation, b_enter_scope);
            }

            int lines_count=vector_count(&b->lines);
            for (int i = 0; i < lines_count; i++){
                ast_to_bytecode_recursive(pointers_vector_get(&b->lines, i), translation, false);
                if(i!=lines_count-1){// result of the last line isn't discarded
                    push_instruction(translation, b_discard);
                }
            }
            if(!keep_scope){
                // the result of evaluating the last line is now on top
                // so the scope object needs to be pushed to the top instead
                push_uint_instruction(translation, b_push_to_top, 1);
                push_instruction(translation, b_leave_scope);
            }

            break;
        }
        case e_name:
        {
            name* n=(name*)exp;
            push_string_load(translation, n->value);
            push_uint_instruction(translation, b_get, 0);
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
            #define OP(operator, instruction) \
                if(strcmp(u->op, operator)==0) { \
                    push_uint_instruction(translation, instruction, 0); \
                    break; \
                }
            OP("+",  b_add) \
            OP("-",  b_subtract) \
            OP("*",  b_multiply) \
            OP("/",  b_divide) \
            OP("//", b_divide_floor) \
            OP("%",  b_modulo)
            #undef OP
            // fallback if no operator was matched
            push_string_load(translation, u->op);
            push_uint_instruction(translation, b_binary, 0);
            break;
        }
        case e_prefix:
        {
            prefix* p=(prefix*)exp;
            ast_to_bytecode_recursive(p->right, translation, false);

            if(strcmp(p->op, "!")==0){
                push_uint_instruction(translation, b_not, 0);
            }else if(strcmp(p->op, "-")==0){
                push_uint_instruction(translation, b_minus, 0);
            } else {
                push_string_load(translation, p->op);
                push_uint_instruction(translation, b_prefix, 0);
            }

            break;
        }
        case e_conditional:
        {
            conditional* c=(conditional*)exp;
            int conditional_end=labels_count++;
            int on_false=labels_count++;

            ast_to_bytecode_recursive(c->condition, translation, false);
            push_uint_instruction(translation, b_jump_not, on_false);// if condition is false jump to on_false label

            ast_to_bytecode_recursive(c->ontrue, translation, false);
            push_uint_instruction(translation, b_jump, conditional_end);// jump over the on_false block to the end of conditional

            push_uint_instruction(translation, b_label, on_false);
            ast_to_bytecode_recursive(c->onfalse, translation, false);

            push_uint_instruction(translation, b_label, conditional_end);

            break;
        }
        case e_function_declaration:
        {
            function_declaration* d=(function_declaration*)exp;
            int arguments_count=vector_count(&d->arguments);
            int sub_program_index=(translation->sub_programs.position/sizeof(BytecodeProgram));

            PreFunctionArgument argument;
            argument.arguments_count=arguments_count;
            argument.is_variadic=d->variadic;
            Instruction instr={b_function_1};
            instr.pre_function_argument=argument;
            stream_push(&translation->code, &instr, sizeof(Instruction));
            repeat_information(translation);

            BytecodeProgram prog=closure_to_bytecode(d);
            stream_push(&translation->sub_programs, &prog, sizeof(BytecodeProgram));
            push_uint_instruction(translation, b_function_2, sub_program_index);
            break;
        }
        case e_function_call:
        {
            function_call* c=(function_call*)exp;
            int lines_count=vector_count(&c->arguments->lines);
            for (int i = 0; i < lines_count; i++){
                ast_to_bytecode_recursive(pointers_vector_get(&c->arguments->lines, i), translation, false);
            }
            ast_to_bytecode_recursive(c->called, translation, false);
            push_uint_instruction(translation, b_call, lines_count);
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
            push_instruction(translation, b_return);
            break;
        }
        case e_question_mark:
        {
            question_mark* q=(question_mark*)exp;
            ast_to_bytecode_recursive((expression*)q->value, translation, false);
            push_instruction(translation, b_double);
            push_string_load(translation, "error");
            push_instruction(translation, b_table_get);
            int after_return=labels_count++;
            push_uint_instruction(translation, b_jump_not, after_return);
            push_instruction(translation, b_return);
            push_uint_instruction(translation, b_label, after_return);
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
    stream_init(&translation->upvalues, UPVALUES_SIZE);
}

BytecodeProgram translation_to_bytecode(BytecodeTranslation* translation, int expected_arguments){
    BytecodeProgram prog;
    prog.source_file_name=NULL;
    stream_truncate(&translation->code);
    stream_truncate(&translation->information);
    stream_truncate(&translation->constants);
    stream_truncate(&translation->sub_programs);
    prog.code=stream_get_data(&translation->code);
    prog.constants=stream_get_data(&translation->constants);
    prog.constants_size=stream_size(&translation->constants);
    prog.information=stream_get_data(&translation->information);
    prog.sub_programs=stream_get_data(&translation->sub_programs);
    prog.sub_programs_count=translation->sub_programs.position/sizeof(BytecodeProgram);
    prog.expected_arguments=expected_arguments;
    prog.upvalues_count=stream_size(&translation->upvalues)/sizeof(unsigned);
    prog.upvalues=stream_get_data(&translation->upvalues);
    prog.assumptions=NULL;
    return prog;
}

void finish_translation(BytecodeTranslation* translation) {
    push_uint_instruction(translation, b_end, 0);
    repeat_information(translation);
}

BytecodeProgram closure_to_bytecode(function_declaration* d){
    BytecodeTranslation translation;
    bytecode_translation_init(&translation);
    
    int arguments_count=vector_count(&d->arguments);
    for (int i = 0; i < arguments_count; i++){
        argument* arg=pointers_vector_get(&d->arguments, arguments_count-1-i);
        push_string_load(&translation, arg->name);
        push_bool_instruction(&translation, b_set, arg->used_in_closure);
        push_instruction(&translation, b_discard);
    }

    ast_to_bytecode_recursive(d->body, &translation, true);
    finish_translation(&translation);

    return translation_to_bytecode(&translation, arguments_count);
}

BytecodeProgram ast_to_bytecode(expression* exp, bool keep_scope){
    BytecodeTranslation translation;
    bytecode_translation_init(&translation);
    
    ast_to_bytecode_recursive(exp, &translation, keep_scope);
    finish_translation(&translation);

    return translation_to_bytecode(&translation, 0);
}