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
    int casted_size=s->size;
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
        if(e->type==e_name){
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
        if(e->type==e_name){
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
        case e_empty:
            push_instruction(code, b_null, 0);
            break;
        case e_literal:
        {
            literal* l=(literal*)exp;
            switch(l->ltype){
                case _int:
                    // all numbers in duck are floats, so int must be converted to float
                    push_number_load(code, (float)l->ival);
                    break;
                case _float:
                    push_number_load(code, l->fval);
                    break;
                case _string:
                    push_string_load(code, constants, l->sval);
                    break;
            }
            break;
        }
        case e_table_literal:
        {
            block* b=(block*)exp;
            
            push_instruction(code, b_get_scope, 0);
            push_instruction(code, b_table_literal, 0);
            push_instruction(code, b_set_scope, 0);

            int last_index=0;
            for (int i = 0; i < vector_total(&b->lines); i++){
                expression* line=vector_get(&b->lines, i);
                ast_to_bytecode_recursive(line, code, constants, 1);
                if(line->type!=e_assignment){
                    // if the expression isn't assignment use last index as a key
                    // arr=[5, 4] => arr[0=5, 1=2]
                    push_number_load(code, (float)last_index++);
                    push_instruction(code, b_set, 0);
                }
                push_instruction(code, b_discard, 0);
            }

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
                ast_to_bytecode_recursive(vector_get(&b->lines, i), code, constants, 1);
                if(i!=lines_count-1){// leave only the last line on the stack
                    push_instruction(code, b_discard, 0);
                }
            }
            break;
        }
        case e_name:
        {
            name* n=(name*)exp;
            push_string_load(code, constants, n->value);
            instruction get={b_get, 0};
            stream_push(code, &get, sizeof(instruction));
            break;
        }
        case e_assignment:
        {
            assignment* a=(assignment*)exp;
            ast_to_bytecode_recursive(a->right, code, constants, keep_scope);
            bytecode_path_set(code, constants, *a->left);
            break;
        }
        case e_unary:
        {
            unary* u=(unary*)exp;

            ast_to_bytecode_recursive(u->right, code, constants, keep_scope);
            ast_to_bytecode_recursive(u->left, code, constants, keep_scope);
            push_string_load(code, constants, u->op);

            push_instruction(code, b_unary, 0);
            break;
        }
        case e_prefix:
        {
            prefix* p=(prefix*)exp;
            ast_to_bytecode_recursive(p->right, code, constants, keep_scope);

            push_string_load(code, constants, p->op);
            push_instruction(code, b_prefix, 0);
            break;
        }
        case e_conditional:
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
        case e_function_declaration:
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
        case e_function_call:
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
        case e_function_return:
        {
            function_return* r=(function_return*)exp;
            ast_to_bytecode_recursive((expression*)r->value, code, constants, keep_scope);
            push_instruction(code, b_return, 0);
            break;
        }
        case e_path:
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

#define X(i) || instr==i
bool pushes_to_stack(instruction_type instr){
    return !(false
    X(b_end)
    X(b_discard)
    X(b_get_scope)
    X(b_label)
    X(b_jump)
    X(b_jump_not)
    );
}
bool changes_flow(instruction_type instr){
    return false
    X(b_return)
    X(b_label)
    X(b_jump)
    X(b_jump_not);
}
bool changes_scope(instruction_type instr){
    return false
    X(b_return)
    X(b_get_scope)
    X(b_set_scope);
}
#undef X

#define X(t, result) case t: return result;
int gets_from_stack(instruction instr){
    switch(instr.type){
        X(b_discard, 1)
        X(b_swap, 1)
        X(b_function, 1 )
        X(b_return, 1)
        X(b_set_scope, 1)
        X(b_get, instr.argument+1)
        X(b_set, instr.argument+2)
        X(b_call, instr.argument+1)
        X(b_unary, 3)
        X(b_prefix, 2)
        default: return 0;
    }
}
#undef X

bool is_path_part(instruction instr){
    return (instr.type==b_get && instr.argument!=0) || instr.type==b_load_string;
}

int path_length(const instruction* code,  int path_start){
    int p=1;
    for(; is_path_part(code[path_start-p]); p++);
    return p;
}

bool instructions_equal(instruction a, instruction b){
    return a.type==b.type && a.argument == b.argument;
}

bool paths_equal(const instruction* code, int path1, int path2){
    int p=0;
    while(is_path_part(code[path1+p]) && is_path_part(code[path2+p])){
        if(!instructions_equal(code[path1+p], code[path2+p])){
            return false;
        }
        p--;
    }
    return true;
}

int count_instructions(instruction* code){
    int p=0;
    for(; code[p].type!=b_end; p++);
    return p;
}

#define LOG_BYTECODE_OPTIMISATIONS 1
#define LOG_CHANGE(message) \
    if(LOG_BYTECODE_OPTIMISATIONS){ \
        USING_STRING(stringify_bytecode(prog), \
        printf(message, str)); \
    }

void move_instructions(bytecode_program* prog, int starting_index, int movement){
    int initial_count=count_instructions(prog->code);
    int moved_count=initial_count+movement;
    instruction* new_code=malloc((moved_count+1)*sizeof(instruction));

    if(LOG_BYTECODE_OPTIMISATIONS)
        printf("move_instructions(%i, %i)\n", starting_index, movement);

    for(int i=0; i<moved_count; i++){
        if(i<=starting_index+movement){
            new_code[i]=prog->code[i];
        } else {
            new_code[i]=prog->code[i-movement];
        }
    }
    instruction end_instruction={b_end, 0};
    new_code[moved_count]=end_instruction;
    prog->code=new_code;
}

/*
What this function does is:
If there is an operation that pushes to stack and the operation that gets the value pushed by it is not before "end"
then a swap instruction must be added before end so the top of the stack is still same as before "start"
returns true if swap operation was added
*/
bool keep_stack_top_unchanged(bytecode_program* prog, int start, int end){
    // count how many objects are pushed to the stack and how many are taken from it
    int counter=0;
    for(int pointer=start; pointer<end; pointer++){
        counter+=pushes_to_stack(prog->code[pointer].type);
        counter-=gets_from_stack(prog->code[pointer]);
    }
    if(counter>0){
        // add swap instruction that swaps "counter" amount of items
        move_instructions(prog, end-1, 1);
        instruction swap_instruction={b_swap, counter};
        prog->code[end]=swap_instruction;
        LOG_CHANGE("added swap:\n%s\n")
        return true;
    } else{
        return false;
    }
}

void optimise_bytecode(bytecode_program* prog){
    LOG_CHANGE("Unoptimised bytecode:\n%s\n");
    for(int pointer=count_instructions(prog->code); pointer>=0; pointer--){
        if(prog->code[pointer].type==b_set && prog->code[pointer+1].type==b_discard){
            if(LOG_BYTECODE_OPTIMISATIONS)
                printf("I found a set instruction: %i\n", pointer);
            bool first_get_removal=true;
            bool used=false;
            for(int search_pointer=pointer+2; prog->code[search_pointer].type!=b_end; search_pointer++){
                if(changes_scope(prog->code[search_pointer].type)){
                    // we optimised all gets in this scope so the varaible isn't needed anymore
                    break;
                }
                if(changes_flow(prog->code[search_pointer].type)){
                    used=true;
                    break;
                }
                if(prog->code[search_pointer].type==b_get && paths_equal(prog->code, pointer-1, search_pointer-1)){
                    int get_path_length=path_length(prog->code, search_pointer);

                    if(keep_stack_top_unchanged(prog, pointer+2, search_pointer+1-get_path_length)){
                        search_pointer++;// if swap instruction was added search needs to be moved by one to point at the correct instruction
                    }
                    move_instructions(prog, search_pointer, -get_path_length);// remove get path instructions
                    if(first_get_removal){
                        move_instructions(prog, pointer+1, -1);// remove discard instruction
                        LOG_CHANGE("removed get and discard instructions:\n%s\n");
                        first_get_removal=false;
                    } else{
                        move_instructions(prog, pointer, 1);
                        instruction double_instruction={b_double, 0};
                        prog->code[pointer+1]=double_instruction;
                        LOG_CHANGE("removed get and added double instruction:\n%s\n")
                    }
                }
            }
            if(!used){
                move_instructions(prog, pointer, -path_length(prog->code, pointer));// remove set path instructions
            }
        }
    }
    LOG_CHANGE("Optimised bytecode:\n%s\n");
}