#include "bytecode.h"

#define CODE_SIZE 50
#define CONSTANTS_SIZE 50

int labels_count=0;

char* INSTRUCTION_NAMES[]={
    "end",
    "discard",
    "swap",
    "load_string",
    "load_number",
    "table_literal",
    "function",
    "return",
    "get_scope",
    "set_scope",
    "label",
    "jump",
    "jump_not",
    "get",
    "set",
    "call",
    "unary",
    "prefix"
};

// creates string variable str, executes body and frees the string afterwards
#define USING_STRING(string_expression, body) { char* str=string_expression; body; free(str); }

void ast_to_bytecode_recursive(expression* exp, stream* code, stream* constants, int keep_scope);

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

            int function_end=labels_count++;
            int function_beginning=labels_count++;

            push_instruction(code, b_jump, function_end);// jump over the function
            push_instruction(code, b_label, function_beginning);// this is where function object will point

            ast_to_bytecode_recursive((expression*)d->body, code, constants, keep_scope);

            push_instruction(code, b_return, 0);
            push_instruction(code, b_label, function_end);

            int arguments_count=vector_total(d->arguments);
            for (int i = 0; i < arguments_count; i++){
                push_string_load(code, constants, ((name*)vector_get(d->arguments, i))->value);
            }
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

void push(stack* stack, object* o){
    o->ref_count++;
    stack_push(stack, (const void*)(&o));
}

object* pop(stack* stack){
    void* pop_result=stack_pop(stack);
    object* o=*((object**)pop_result);
    o->ref_count--;
    return o;
}

char* stringify_object_stack(const stack* s){
    int pointer=0;
    int string_end=0;
    int result_size=64;
    char* result=calloc(64, sizeof(char));
    CHECK_ALLOCATION(result);
    
    object** casted_items=(object**)s->items;
    while(pointer<s->top){
        char* stringified_item=stringify(*(casted_items+pointer));
        int stringified_length=strlen(stringified_item);
        if(result_size-string_end+2<=stringified_length){// +2 for separator
            result_size*=2;
            result=realloc(result, result_size);
        }
        strncat(result, stringified_item, result_size);
        strncat(result, ", ", result_size);
        string_end+=stringified_length+2;
        pointer++;
    }
    result[string_end]='\0';
    return result; 
}

object* execute_bytecode(instruction* code, void* constants, table* scope){
    int pointer=0;// points to the current instruction

    stack object_stack;
    stack_init(&object_stack, sizeof(object*), STACK_SIZE);
    push(&object_stack, (object*)new_null());

    stack return_stack;
    stack_init(&return_stack, sizeof(table*), STACK_SIZE);

    while(code[pointer].type!=b_end){
        instruction instr=code[pointer];
        switch(instr.type){
            case b_discard:
                // remove item from the stack and delete it if it's not referenced
                delete_unreferenced(pop(&object_stack));
                break;
            case b_swap:
            {
                object* a=pop(&object_stack);
                object* b=pop(&object_stack);
                push(&object_stack, a);
                push(&object_stack, b);
                break;
            }
            case b_load_string:
            {
                string* s=new_string();
                s->value=strdup(((char*)constants)+instr.argument);
                push(&object_stack, (object*)s);
                break;
            }
            case b_load_number:
            {
                number* n=new_number();
                memcpy (&n->value, &instr.argument, sizeof n->value);
                push(&object_stack, (object*)n);
                break;
            }
            case b_table_literal:
            {
                table* t=new_table();
                push(&object_stack, (object*)t);
                break;
            }
            case b_get:
            {
                object* key=pop(&object_stack);
                if(instr.argument){
                    object* indexed=pop(&object_stack);
                    USING_STRING(stringify(key),
                        push(&object_stack, get(indexed, str)));
                    delete_unreferenced(indexed);
                } else {
                    USING_STRING(stringify(key), 
                        push(&object_stack, get((object*)scope, str)));
                }
                delete_unreferenced(key);
                break;
            }
            case b_set:
            {
                object* key=pop(&object_stack);
                object* value=pop(&object_stack);
                if(instr.argument){
                    object* indexed=pop(&object_stack);
                    set(indexed, stringify(key), value);
                    delete_unreferenced(indexed);
                } else {
                    set((object*)scope, stringify(key), value);
                }
                delete_unreferenced(key);
                push(&object_stack, value);
                break;
            }
            case b_get_scope:
            {
                push(&object_stack, (object*)scope);
                break;
            }
            case b_set_scope:
            {
                delete_unreferenced((object*)scope);
                object* o=pop(&object_stack);
                if(o->type!=t_table){
                    USING_STRING(stringify(o),
                        ERROR(WRONG_ARGUMENT_TYPE, "b_set_scope: %s isn't a table, number of instruction is: %i\n", str, pointer));
                } else {
                    scope=(table*)o;
                }
                break;
            }
            case b_unary:
            {
                object* op=pop(&object_stack);
                object* a=pop(&object_stack);
                object* b=pop(&object_stack); 
                push(&object_stack, operator(a, b, stringify(op)));
                delete_unreferenced(op);
                delete_unreferenced(a);
                delete_unreferenced(b);
                break;
            }
            case b_prefix:
            {
                object* op=pop(&object_stack);
                object* a=pop(&object_stack);
                object* b=(object*)new_null();// replace second argument with null
                push(&object_stack, operator(a, b, stringify(op)));
                delete_unreferenced(op);
                delete_unreferenced(a);
                // operator might store or return the second argument so it can't be simply removed
                delete_unreferenced(b);
                break;
            }
            case b_jump_not:
            {
                object* condition=pop(&object_stack);
                if(!is_falsy(condition)){
                    delete_unreferenced(condition);
                    break;// go to the next line
                }
                delete_unreferenced(condition);
                // else go to case label underneath
            }
            case b_jump:
            {
                int destination=search_for_label(code, instr.argument);
                if(destination>0){
                    pointer=destination;
                } else {
                    ERROR(WRONG_ARGUMENT_TYPE, "Incorrect jump label %li, number of instruction is: %i\n", instr.argument, pointer);
                }
                break;
            }
            case b_function:
            {
                function* f=new_function();
                f->enclosing_scope=scope;
                scope->ref_count++;// remember to check the enclosing scope in destructor
                object* arguments_count_object=pop(&object_stack);
                if(arguments_count_object->type!=t_number){
                    ERROR(WRONG_ARGUMENT_TYPE, "Number of function arguments isn't present or has a wrong type, number of instruction is: %i\n", pointer);
                    break;
                }
                int arguments_count=(int)((number*)arguments_count_object)->value;
                for (int i = 0; i < arguments_count; i++){
                    object* argument=pop(&object_stack);
                    vector_add(&f->argument_names, stringify(argument));
                    delete_unreferenced(argument);
                }
                f->is_native=0;
                f->data=(void*)instr.argument;
                push(&object_stack, (object*)f);
                break;
            }
            case b_call:
            {
                object* o=pop(&object_stack);
                if(o->type==t_null){
                    ERROR(WRONG_ARGUMENT_TYPE, "Called function is null, number of instruction is: %i\n", pointer);
                    break;
                } else if(o->type!=t_function){
                    USING_STRING(stringify(o),
                        ERROR(WRONG_ARGUMENT_TYPE, "Called object \"%s\" is not a function, number of instruction is: %i\n", str, pointer));
                    break;
                }
                function* f=(function*)o;

                table* function_scope=new_table();
                if(f->enclosing_scope!=NULL){
                    setup_scope((object*)function_scope, (object*)f->enclosing_scope);
                } else {
                    setup_scope((object*)function_scope, (object*)scope);
                }
                if(f->is_native){
                    
                    for (int i = 0; i < vector_total(&f->argument_names); i++){
                        //char buf[16];
                        //itoa(i, buf, 10);
                        char* argument_name=vector_get(&f->argument_names, i);
                        object* argument_value=pop(&object_stack);
                        set((object*)function_scope, argument_name, argument_value);
                    }
                    push(&object_stack,  call((object*)f, function_scope));
                    delete_unreferenced((object*)function_scope);
                } else {
                    int destination=search_for_label(code, (int)f->data);
                    if(destination>0){
                        number* return_point=new_number();
                        return_point->value=pointer;
                        set((object*)function_scope, "return_point", (object*)return_point);
                        push(&return_stack, (object*)scope);
                        
                        scope=function_scope;
                        pointer=destination;
                        for (int i = 0; i < vector_total(&f->argument_names); i++){
                            //char buf[16];
                            //itoa(i, buf, 10);
                            char* argument_name=vector_get(&f->argument_names, i);
                            object* argument_value=pop(&object_stack);
                            set((object*)scope, argument_name, argument_value);
                        }
                    } else {
                        ERROR(WRONG_ARGUMENT_TYPE, "Incorrect function jump label %i, number of instruction is: %i\n", (int)f->data, pointer);
                        delete_unreferenced((object*)function_scope);
                    }
               }
               break;
            }
            case b_return:
            {
                object* return_point=get((object*)scope, "return_point");
                if(return_point->type!=t_number){
                    USING_STRING(stringify(return_point), 
                        ERROR(WRONG_ARGUMENT_TYPE, "Return point (%s) is not a number, number of instruction is: %i\n", str, pointer));
                } else {
                    pointer=((number*)return_point)->value;
                    object* return_scope=pop(&return_stack);
                    if(return_scope->type!=t_table){
                        USING_STRING(stringify(return_scope), 
                            ERROR(WRONG_ARGUMENT_TYPE, "Return scope (%s) is not a table, number of instruction is: %i\n", str, pointer));
                    } else {
                        delete_unreferenced((object*)scope);
                        scope=(table*)return_scope;
                    }
                }
                break;
            }
            case b_label:
                break;
            default:
                ERROR(WRONG_ARGUMENT_TYPE, "Uncatched bytecode instruction type: %i, number of instruction is: %i\n", instr.type, pointer);
        }
        pointer++;
    }
    return pop(&object_stack);
}

void stringify_instruction(bytecode_program prog, char* destination, instruction instr, int buffer_count){
    switch(instr.type){
        case b_end:
        case b_discard:
        case b_swap:
        case b_get_scope:
        case b_set_scope:
        case b_call:
        case b_return:
        case b_unary:
        case b_prefix:
            snprintf(destination, buffer_count, "%s\n", INSTRUCTION_NAMES[instr.type]);// these instructions doesn't use the argument
            break;
        case b_load_number:
            snprintf(destination, buffer_count, "%s %f\n", INSTRUCTION_NAMES[instr.type], *((float*)&instr.argument));
            break;
        case b_load_string:
            snprintf(destination, buffer_count, "%s %li \"%s\"\n", INSTRUCTION_NAMES[instr.type], instr.argument, ((char*)prog.constants)+instr.argument);// displays string value
            break;
        default:
            snprintf(destination, buffer_count, "%s %li\n", INSTRUCTION_NAMES[instr.type], instr.argument);
    }
}

char* stringify_bytecode(bytecode_program prog){
    int pointer=0;
    int string_end=0;
    int result_size=64;
    char* result=calloc(64, sizeof(char));
    CHECK_ALLOCATION(result);

    while(prog.code[pointer].type!=b_end){
        char stringified_instruction[64];
        stringify_instruction(prog, (char*)&stringified_instruction, prog.code[pointer], 64);
        int stringified_length=strlen(stringified_instruction);

        if(result_size-string_end<=stringified_length){
            result_size*=2;
            result=realloc(result, result_size);
        }
        strncat(result, stringified_instruction, result_size);
        string_end+=strlen(stringified_instruction);
        pointer++;
    }
    result[string_end]='\0';
    return result; 
}