#include "ast_to_bytecode.h"

#define CODE_SIZE 32
#define CONSTANTS_SIZE 32
#define SUB_PROGRAMS_SIZE 4
#define UPVALUES_SIZE 8

int labels_count=0;

void ast_to_bytecode_recursive(Expression* expression, BytecodeTranslation* translation, bool keep_scope);

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

InstructionInformation information_from_ast(BytecodeTranslation* translation, Expression* expression){
    InstructionInformation info;
    info.line=expression->line_number;
    info.column=expression->column_number;
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

void table_literal_key(BytecodeTranslation* translation, Expression* expression) {
    switch(expression->type){
        case e_name:
            push_string_load(translation, ((Name*)expression)->value);
            break;
        case e_self_indexer:
            ast_to_bytecode_recursive(((SelfIndexer*)expression)->right, translation, 0);
            break;
        default: 
            // TODO: More informative message
            THROW_ERROR(WRONG_ARGUMENT_TYPE, "Incorrect expression in table literal key.\n");
            break;
    }
}

void bytecode_get(BytecodeTranslation* translation, Expression* expression){
    switch(expression->type){
        case e_name:
        {
            Name* n=(Name*)expression;
            uint push_position=push_string_load(translation, n->value);
            if(is_upvalue(translation, n->value)){
                stream_push(&translation->upvalues, &push_position, sizeof(unsigned));
            }
            push_instruction(translation, b_get);
            break;
        }
        case e_member_access:
        {
            MemberAccess* m=(MemberAccess*)expression;
            bytecode_get(translation, m->left);
            push_string_load(translation, m->right->value);
            push_instruction(translation, b_table_get);
            break;
        }
        case e_indexer:
        {
            Indexer* i=(Indexer*)expression;
            bytecode_get(translation, i->left);
            ast_to_bytecode_recursive(i->right, translation, false);
            push_instruction(translation, b_table_get);
            break;
        }
        default:;
    }
}

void bytecode_set(BytecodeTranslation* translation, Expression* expression, bool used_in_closure){
    switch(expression->type){
        case e_name:
        {
            Name* n=(Name*)expression;
            uint push_position=push_string_load(translation, n->value);
            if(is_upvalue(translation, n->value)){
                stream_push(&translation->upvalues, &push_position, sizeof(unsigned));
            }
            push_bool_instruction(translation, b_set, used_in_closure);
            break;
        }
        case e_member_access:
        {
            MemberAccess* m=(MemberAccess*)expression;
            bytecode_get(translation, m->left);
            push_string_load(translation, m->right->value);
            push_instruction(translation, b_table_set);
            break;
        }
        case e_indexer:
        {
            Indexer* i=(Indexer*)expression;
            bytecode_get(translation, i->left);
            ast_to_bytecode_recursive(i->right, translation, false);
            push_instruction(translation, b_table_set);
            break;
        }
        default:
            USING_STRING(stringify_expression(expression, 0),
                THROW_ERROR(BYTECODE_ERROR, "Incorrect expression on left hand of assignment: \n%s", str);)
    }
}

void ast_to_bytecode_recursive(Expression* expression, BytecodeTranslation* translation, bool keep_scope){
    translation->last_information=information_from_ast(translation, expression);

    switch(expression->type){
        case e_expression:
        case e_macro_declaration:
        case e_macro:
        case e_empty:
        case e_null_literal:
            push_uint_instruction(translation, b_null, 0);
            break;
        case e_float_literal:
            push_float_load(translation, ((FloatLiteral*)expression)->value);
            break;
        case e_int_literal:
            push_int_load(translation, ((IntLiteral*)expression)->value);
            break;
        case e_string_literal:
            push_string_load(translation, ((StringLiteral*)expression)->value);
            break;
        case e_table_literal:
        {
            TableLiteral* b=(TableLiteral*)expression;
            int lines_count=vector_count(&b->lines);
            push_uint_instruction(translation, b_table_literal, 0);
            if(lines_count>0){
                int last_index=0;
                for (int i = 0; i < lines_count; i++){
                    Expression* line=pointers_vector_get(&b->lines, i);
                    if(line->type==e_assignment){
                        Assignment* assignment_line=((Assignment*)line);
                        ast_to_bytecode_recursive(assignment_line->right, translation, true);
                        table_literal_key(translation, assignment_line->left);// assuming that assignment_line inside of Table always has one item
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
            Block* b=(Block*)expression;

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
        case e_assignment:
        {
            Assignment* a=(Assignment*)expression;
            ast_to_bytecode_recursive(a->right, translation, false);
            bytecode_set(translation, a->left, a->used_in_closure);
            break;
        }
        case e_binary:
        {
            Binary* u=(Binary*)expression;

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
            Prefix* p=(Prefix*)expression;
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
            Conditional* c=(Conditional*)expression;
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
            FunctionDeclaration* d=(FunctionDeclaration*)expression;
            int arguments_count=vector_count(&d->arguments);
            int sub_program_index=(translation->sub_programs.position/sizeof(BytecodeProgram*));

            FunctionArgument argument;
            argument.arguments_count=arguments_count;
            argument.is_variadic=d->variadic;
            Instruction instr={b_function_1};
            instr.function_argument=argument;
            stream_push(&translation->code, &instr, sizeof(Instruction));
            repeat_information(translation);

            BytecodeProgram* program=ast_function_to_bytecode(d);
            stream_push(&translation->sub_programs, &program, sizeof(BytecodeProgram*));
            push_uint_instruction(translation, b_function_2, sub_program_index);
            break;
        }
        case e_function_call:
        {
            FunctionCall* c=(FunctionCall*)expression;
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
            ast_to_bytecode_recursive(((Parentheses*)expression)->value, translation, false);
            break;
        }
        case e_function_return:
        {
            FunctionReturn* r=(FunctionReturn*)expression;
            ast_to_bytecode_recursive((Expression*)r->value, translation, false);
            push_instruction(translation, b_return);
            break;
        }
        case e_return_if_error:
        {
            ReturnIfError* r=(ReturnIfError*)expression;
            ast_to_bytecode_recursive((Expression*)r->value, translation, false);
            push_instruction(translation, b_double);
            push_string_load(translation, "is_error");
            push_instruction(translation, b_get);
            push_uint_instruction(translation, b_call, 1);
            int after_return=labels_count++;
            push_uint_instruction(translation, b_jump_not, after_return);
            push_instruction(translation, b_return);
            push_uint_instruction(translation, b_label, after_return);
            break;
        }
        case e_name:
        case e_member_access:
        case e_indexer:
            bytecode_get(translation, expression);
            break;
        default:
        {
            THROW_ERROR(WRONG_ARGUMENT_TYPE, "Uncatched expression Instruction type: %i\n", expression->type);
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

BytecodeProgram* translation_to_bytecode(BytecodeTranslation* translation, int expected_arguments){
    BytecodeProgram* program=malloc(sizeof(BytecodeProgram));
    program->source_file_name=NULL;
    stream_truncate(&translation->code);
    stream_truncate(&translation->constants);
    stream_truncate(&translation->information);
    stream_truncate(&translation->sub_programs);
    program->code=stream_get_data(&translation->code);
    program->constants=stream_get_data(&translation->constants);
    program->constants_size=stream_size(&translation->constants);
    program->information=stream_get_data(&translation->information);
    program->sub_programs=stream_get_data(&translation->sub_programs);
    program->sub_programs_count=translation->sub_programs.position/sizeof(BytecodeProgram*);
    program->expected_arguments=expected_arguments;
    program->upvalues_count=stream_size(&translation->upvalues)/sizeof(unsigned);
    program->upvalues=stream_get_data(&translation->upvalues);
    program->assumptions=NULL;
    return program;
}

void finish_translation(BytecodeTranslation* translation) {
    push_uint_instruction(translation, b_end, 0);
    repeat_information(translation);
}

BytecodeProgram* ast_function_to_bytecode(FunctionDeclaration* d){
    BytecodeTranslation translation;
    bytecode_translation_init(&translation);
    translation.last_information=information_from_ast(&translation, (Expression*)d);
    
    int arguments_count=vector_count(&d->arguments);
    for (int i = 0; i < arguments_count; i++){
        Argument* arg=pointers_vector_get(&d->arguments, arguments_count-1-i);
        push_string_load(&translation, arg->name);
        push_bool_instruction(&translation, b_set, arg->used_in_closure);
        push_instruction(&translation, b_discard);
    }

    ast_to_bytecode_recursive(d->body, &translation, true);
    finish_translation(&translation);

    return translation_to_bytecode(&translation, arguments_count);
}

BytecodeProgram* ast_to_bytecode(Expression* expression, bool keep_scope){
    BytecodeTranslation translation;
    bytecode_translation_init(&translation);
    
    ast_to_bytecode_recursive(expression, &translation, keep_scope);
    finish_translation(&translation);

    return translation_to_bytecode(&translation, 0);
}