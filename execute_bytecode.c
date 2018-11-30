#include "execute_bytecode.h"

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
            case b_null:
            {
                null* n=new_null();
                push(&object_stack, (object*)n);
                break;
            }
            case b_get:
            {
                object* key=pop(&object_stack);
                object* indexed;
                if(instr.argument){
                    indexed=pop(&object_stack);
                } else {
                    indexed=(object*)scope;
                }
                /*object* get_override=get(indexed, "get");
                if(get_override->type!=t_null){
                    if(get_override->type==t_function){
                        function* f=(function*)get_override;
                        if(f->f_type==f_bytecode){
                            //move pointer to the label
                            //return position is this line
                        }
                    } else if(get_override->type==t_table){
                        // but to get call_override you'd need to access get_override's get override which could be bytecode function
                        // this bytecode function would need to go back here and return the call_override which could again have it's own call override and get override
                        object* call_override=get(get_override, "call");
                        while(call_override->type!=t_function) {
                            object* call_override=get(get_override, "call");
                        }
                    }
                }*/
                USING_STRING(stringify(key), 
                        push(&object_stack, get(indexed, str)));
                if(instr.argument){
                    // delete indexed if it isn't the scope
                    delete_unreferenced(indexed);
                }
                delete_unreferenced(key);
                break;
            }
            case b_set:
            {
                object* key=pop(&object_stack);
                object* indexed;
                if(instr.argument){
                    indexed=pop(&object_stack);
                } else {
                    indexed=(object*)scope;
                }
                object* value=pop(&object_stack);
                set(indexed, stringify(key), value);
                if(instr.argument){
                    // delete indexed if it isn't the scope
                    delete_unreferenced(indexed);
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
                f->arguments_count=(int)((number*)arguments_count_object)->value;
                f->f_type=f_bytecode;
                f->label=instr.argument;
                push(&object_stack, (object*)f);
                break;
            }
            case b_call:
            {
                object* o=pop(&object_stack);
                function* f=(function*)cast(o, t_function);
                
                if(f->f_type==f_native){
                    vector arguments;
                    vector_init(&arguments);
                    for (int i = 0; i < f->arguments_count; i++){
                        vector_add(&arguments, pop(&object_stack));
                    }
                    push(&object_stack, f->pointer(arguments));
                    vector_free(&arguments);
                } else if(f->f_type==f_bytecode){
                    table* function_scope=new_table();
                    if(f->enclosing_scope!=NULL){
                        setup_scope((object*)function_scope, (object*)f->enclosing_scope);
                    } else {
                        setup_scope((object*)function_scope, (object*)scope);
                    }
                    int destination=search_for_label(code, f->label);
                    if(destination>0){
                        number* return_point=new_number();
                        return_point->value=pointer;
                        set((object*)function_scope, "return_point", (object*)return_point);
                        push(&return_stack, (object*)scope);
                        
                        scope=function_scope;
                        pointer=destination;
                    } else {
                        ERROR(WRONG_ARGUMENT_TYPE, "Incorrect function jump label %i, number of instruction is: %i\n", f->label, pointer);
                    }
               } else if(f->f_type==f_ast) {
                   ERROR(NOT_IMPLEMENTED, "Can't call ast function from bytecode, number of instruction is: %i\n", pointer);
               } else {
                    ERROR(INCORRECT_OBJECT_POINTER, "Incorrect function pointer, number of instruction is: %i\n", pointer);
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