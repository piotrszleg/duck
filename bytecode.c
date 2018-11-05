#include "bytecode.h"

#define CODE_SIZE 200
#define CONSTANTS_SIZE 200

int labels_count=0;

char* INSTRUCTION_NAMES[]={
    "end",
    "discard",
    "swap",
    "load_string",
    "load_number",
    "table_literal",
    "function_literal",
    "get_scope",
    "set_scope",
    "label",
    "jump",
    "jump_not",
    "get",
    "set",
    "call",
    "operator",
};

void init_stream(stream* s, size_t size){
    s->data=malloc(size);
    CHECK_ALLOCATION(s->data);
    s->position=0;
    s->size=size;
}

int stream_push(stream* s, void* data_pointer, size_t size){
    if(size>s->size-s->position){
        ERROR(BUFFER_TOO_SMALL, "stream_push: stream size is too small");
    }
    int push_position=s->position;
    // void pointers can't be incremented normally so they need to be casted
    char* destination=(char*)s->data;
    destination+=push_position;

    memcpy(destination, data_pointer, size);
    s->position+=size;
    return push_position;
}

void stream_truncate(stream* s){
    s->data=realloc(s->data, s->position);
    CHECK_ALLOCATION(s->data);
}

bytecode_program ast_to_bytecode(expression* exp, int keep_scope){
    stream constants;
    stream code;
    init_stream(&constants, CONSTANTS_SIZE);
    init_stream(&code, CODE_SIZE);
    CHECK_ALLOCATION(malloc(8));
    ast_to_bytecode_recursive(exp, &code, &constants, keep_scope);
    CHECK_ALLOCATION(malloc(8));
    instruction instr={b_end, 0};
    stream_push(&code, &instr, sizeof(instruction));
    bytecode_program prog;
    CHECK_ALLOCATION(malloc(8));
    stream_truncate(&code);
    CHECK_ALLOCATION(malloc(8));
    stream_truncate(&constants);
    CHECK_ALLOCATION(malloc(8));
    prog.code=code.data;
    prog.constants=constants.data;
    return prog;
}

void push_string_load(stream* code, stream* constants, const char* string_constant){
    int constant_position=stream_push(constants, string_constant, (strlen(string_constant)+1)*sizeof(char*));
    instruction load={b_load_string, constant_position};
    stream_push(code, &load, sizeof(instruction));
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

void push_instruction(stream* code, instruction_type type, long value){
    instruction instr={type, value};
    stream_push(code, &instr, sizeof(instruction));
}

void ast_to_bytecode_recursive(expression* exp, stream* code, stream* constants, int keep_scope){
    switch(exp->type){
        case _empty:
            break;
        case _literal:
        {
            literal* l=(literal*)exp;
            switch(l->ltype){
                case _int:
                {
                    float casted=(float)l->ival;// all numbers in duck are floats, so int must be converted to float
                    long argument;
                    memcpy (&argument, &casted, sizeof argument);
                    push_instruction(code, b_load_number, argument);
                    break;
                }
                case _float:
                {
                    long argument;
                    memcpy (&argument, &l->fval, sizeof argument);
                    push_instruction(code, b_load_number, argument);
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

            for (int i = 0; i < vector_total(&b->lines); i++){
                ast_to_bytecode_recursive(vector_get(&b->lines, i), code, constants, 1);
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
            push_string_load(code, constants, &n->value);
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
            instruction unary={b_operator, 0};
            stream_push(code, &unary, sizeof(instruction));
            break;
        }
        case _prefix:
        {
            prefix* p=(prefix*)exp;
            ast_to_bytecode_recursive(p->right, code, constants, keep_scope);

            push_string_load(code, constants, p->op);
            instruction unary={b_operator, 1};
            stream_push(code, &unary, sizeof(instruction));
            break;
        }
        case _conditional:
        {
            conditional* c=(conditional*)exp;
            ast_to_bytecode_recursive(c->condition, code, constants, keep_scope);

            instruction jmp_not={b_jump_not, labels_count++};
            stream_push(code, &jmp_not, sizeof(instruction));

            ast_to_bytecode_recursive(c->ontrue, code, constants, keep_scope);
            instruction jmp_after_ontrue={b_jump, labels_count++};
            stream_push(code, &jmp_after_ontrue, sizeof(instruction));

            instruction jmp_not_label={b_label, jmp_not.argument};
            stream_push(code, &jmp_not_label, sizeof(instruction));
            ast_to_bytecode_recursive(c->onfalse, code, constants, keep_scope);

            instruction label_after_conditional={b_label, jmp_after_ontrue.argument};
            stream_push(code, &label_after_conditional, sizeof(instruction));

            break;
        }
        case _function_declaration:
        {
            function_declaration* d=(function_declaration*)exp;

            instruction jmp={b_jump, labels_count++};
            stream_push(code, &jmp, sizeof(instruction));
            instruction function_beginning={b_label, labels_count++};
            stream_push(code, &function_beginning, sizeof(instruction));

            ast_to_bytecode_recursive(d->body, code, constants, keep_scope);

            instruction function_end={b_label, jmp.argument};
            stream_push(code, &function_end, sizeof(instruction));
            instruction function_declaration={b_function_literal, function_beginning.argument};
            stream_push(code, &function_declaration, sizeof(instruction));
            break;
        }
        case _function_call:
        {
            // TODO: subscoping
            function_call* c=(function_call*)exp;
            int lines_count=vector_total(&c->arguments->lines);
            for (int i = 0; i < lines_count; i++){
                ast_to_bytecode_recursive(vector_get(&c->arguments->lines, i), code, constants, keep_scope);
            }
            bytecode_path_get(code, constants, *c->function_path);
            instruction call_instr={b_call, lines_count};
            stream_push(code, &call_instr, sizeof(instruction));
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

void* move_pointer(const void* pointer, int number_of_bytes){
    char* casted_pointer=(char*)pointer;
    casted_pointer+=number_of_bytes/sizeof(char*);
    return (void*)casted_pointer;
}

void push(vm_stack* stack, object* o){
    o->ref_count++;
    stack->items[stack->pointer]=o;
    stack->pointer++;
}

object* pop(vm_stack* stack){
    stack->pointer--;
    stack->items[stack->pointer]->ref_count--;
    return stack->items[stack->pointer];
}

int search_for_label(const instruction* code, int label_index){
    int pointer=0;
    while(code[pointer].type!=b_end){
        if(code[pointer].type==b_label && code[pointer].argument==label_index){
            return pointer;
        }
        pointer++;
    }
    return -1;
}

object* execute_bytecode(instruction* code, void* constants, table* scope){
    int pointer=0;
    vm_stack stack;
    stack.pointer=0;
    push(&stack, new_null());
    while(code[pointer].type!=b_end){
        instruction instr=code[pointer];
        switch(instr.type){
            case b_discard:
                // remove item from the stack and delete it if it's not referenced
                garbage_collector_check(pop(&stack));
                break;
            case b_swap:
            {
                object* a=pop(&stack);
                object* b=pop(&stack);
                push(&stack, a);
                push(&stack, b);
                break;
            }
            case b_load_string:
            {
                string* s=new_string();
                s->value=strdup(((char*)constants)+instr.argument);
                push(&stack, s);
                break;
            }
            case b_load_number:
            {
                number* n=new_number();
                memcpy (&n->value, &instr.argument, sizeof n->value);
                push(&stack, n);
                break;
            }
            case b_table_literal:
            {
                table* t=new_table();
                push(&stack, t);
                break;
            }
            case b_get:
            {
                object* key=pop(&stack);
                if(instr.argument){
                    object* indexed=pop(&stack);
                    push(&stack, get(indexed, stringify(key)));
                    object_delete(indexed);
                } else {
                    push(&stack, get(scope, stringify(key)));
                }
                garbage_collector_check(key);
                break;
            }
            case b_set:
            {
                object* key=pop(&stack);
                object* value=pop(&stack);
                if(instr.argument){
                    object* indexed=pop(&stack);
                    set(indexed, stringify(key), value);
                    garbage_collector_check(indexed);
                } else {
                    set(scope, stringify(key), value);
                }
                garbage_collector_check(key);
                push(&stack, value);
                break;
            }
            case b_get_scope:
            {
                push(&stack, scope);
                break;
            }
            case b_set_scope:
            {
                garbage_collector_check(scope);
                scope=pop(&stack);
                break;
            }
            case b_operator:
            {
                object* op=pop(&stack);
                object* a=pop(&stack);
                object* b=pop(&stack); 
                push(&stack, operator(a, b, stringify(op)));
                garbage_collector_check(op);
                garbage_collector_check(a);
                garbage_collector_check(b);
                break;
            }
            case b_jump_not:
            {
                
                object* condition=pop(&stack);
                if(!is_falsy(condition)){
                    break;// go to the next line
                }
                // else go to case label underneath
            }
            case b_jump:
            {
                int destination=search_for_label(code, instr.argument);
                if(destination>0){
                    pointer=destination;
                }
                break;
            }
            case b_call:
            {
                table* function_scope=new_table();
                function* f=pop(&stack);
                /*if(f->enclosing_scope!=NULL){
                    setup_scope((object*)function_scope, (object*)f->enclosing_scope);
                } else {
                    setup_scope((object*)function_scope, (object*)scope);
                }
                if(vector_total(&c->arguments->lines)<vector_total(&f->argument_names)){
                    ERROR(WRONG_ARGUMENT_TYPE, "Not enough arguments in function call.");
                }*/
                for (int i = 0; i < vector_total(&f->argument_names); i++){
                    //char buf[16];
                    //itoa(i, buf, 10);
                    char* argument_name=vector_get(&f->argument_names, i);
                    object* argument_value=pop(&stack);
                    set((object*)function_scope, argument_name, argument_value);
                }
                push(&stack,  call((object*)f, function_scope));
            }
            case b_label:
                break;
            default:
                ERROR(WRONG_ARGUMENT_TYPE, "Uncatched bytecode instruction type: %i\n", instr.type);
        }
        pointer++;
    }
    return pop(&stack);
}

void stringify_instruction(char* destination, instruction inst){
    snprintf(destination, 100, "%s %i\n", INSTRUCTION_NAMES[inst.type], inst.argument);
}

char* stringify_bytecode(instruction* code){
    int pointer=0;
    int string_end=0;
    char* result=calloc(100, sizeof(char));
    CHECK_ALLOCATION(result);
    while(code[pointer].type!=b_end){
        char stringified_instruction[50];
        stringify_instruction(&stringified_instruction, code[pointer]);
        strcat(result, stringified_instruction);
        string_end+=strlen(stringified_instruction);
        pointer++;
    }
    result[string_end]='\0';
    return result; 
}