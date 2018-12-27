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

void push(stack* stack, object o){
    reference(&o);
    stack_push(stack, (const void*)(&o));
}

object pop(stack* stack){
    void* pop_result=stack_pop(stack);
    object* o=pop_result;
    dereference(o);
    return *o;
}

char* stringify_object_stack(const stack* s){
    int pointer=0;
    int string_end=0;
    int result_size=64;
    char* result=calloc(64, sizeof(char));
    CHECK_ALLOCATION(result);
    
    object* casted_items=(object*)s->items;
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

void bytecode_enviroment_init(bytecode_environment* e){
    stack_init(&e->object_stack, sizeof(object), STACK_SIZE);
    push(&e->object_stack, null_const);
    stack_init(&e->return_stack, sizeof(object), STACK_SIZE);
}

object execute_bytecode(bytecode_environment* environment, object scope){
    int* pointer=&environment->pointer;// points to the current instruction

    stack* object_stack=&environment->object_stack;
    stack* return_stack=&environment->return_stack;

    instruction* code=environment->code;
    void* constants=environment->constants;

    while(code[*pointer].type!=b_end){
        instruction instr=code[*pointer];
        environment->line=*pointer;
        switch(instr.type){
            case b_discard:
            {
                // remove item from the stack and delete it if it's not referenced
                object top=pop(object_stack);
                object_deinit(&top);
                break;
            }
            #define INDEX_STACK(index) ((object*)object_stack->items)[object_stack->top-1-(index)]
            case b_swap:
            {
                for(int i=instr.argument-1; i>=0; i--){
                    object swap_temporary=INDEX_STACK(i);
                    INDEX_STACK(i)=INDEX_STACK(i+1);
                    INDEX_STACK(i+1)=swap_temporary;
                }
                
                break;
            }
            case b_double:
            {
                push(object_stack, INDEX_STACK(0));
                break;
            }
            #undef INDEX_STACK
            case b_load_string:
            {
                object s;
                string_init(&s);
                s.text=strdup(((char*)constants)+instr.argument);
                push(object_stack, s);
                break;
            }
            case b_load_number:
            {
                object n;
                number_init(&n);
                memcpy (&n.value, &instr.argument, sizeof n.value);
                push(object_stack, n);
                break;
            }
            case b_table_literal:
            {
                object t;
                table_init(&t);
                push(object_stack, t);
                break;
            }
            case b_null:
            {
                push(object_stack, null_const);
                break;
            }
            case b_get:
            {
                object key=pop(object_stack);
                object indexed;
                if(instr.argument){
                    indexed=pop(object_stack);
                } else {
                    indexed=scope;
                }
                USING_STRING(stringify(key), 
                        push(object_stack, get(indexed, str)));
                if(instr.argument){
                    // delete indexed if it isn't the scope
                    object_deinit(&indexed);
                }
                object_deinit(&key);
                break;
            }
            case b_set:
            {
                object key=pop(object_stack);
                object indexed;
                if(instr.argument){
                    indexed=pop(object_stack);
                } else {
                    indexed=scope;
                }
                object value=pop(object_stack);
                set(indexed, stringify(key), value);
                if(instr.argument){
                    // delete indexed if it isn't the scope
                    object_deinit(&indexed);
                }
                object_deinit(&key);
                push(object_stack, value);
                break;
            }
            case b_get_scope:
            {
                push(object_stack, scope);
                break;
            }
            case b_set_scope:
            {
                object_deinit(&scope);
                object o=pop(object_stack);
                if(o.type!=t_table){
                    USING_STRING(stringify(o),
                        ERROR(WRONG_ARGUMENT_TYPE, "b_set_scope: %s isn't a table, number of instruction is: %i\n", str, *pointer));
                } else {
                    scope=o;
                }
                break;
            }
            case b_unary:
            {
                object op=pop(object_stack);
                object a=pop(object_stack);
                object b=pop(object_stack); 
                push(object_stack, operator(a, b, stringify(op)));
                object_deinit(&op);
                object_deinit(&a);
                object_deinit(&b);
                break;
            }
            case b_prefix:
            {
                object op=pop(object_stack);
                object a=pop(object_stack);
                push(object_stack, operator(a, null_const, stringify(op)));
                object_deinit(&op);
                object_deinit(&a);
                break;
            }
            case b_jump_not:
            {
                object condition=pop(object_stack);
                if(!is_falsy(condition)){
                    object_deinit(&condition);
                    break;// go to the next line
                }
                object_deinit(&condition);
                // else go to case label underneath
            }
            case b_jump:
            {
                int destination=search_for_label(code, instr.argument);
                if(destination>0){
                    *pointer=destination;
                } else {
                    ERROR(WRONG_ARGUMENT_TYPE, "Incorrect jump label %li, number of instruction is: %i\n", instr.argument, *pointer);
                }
                break;
            }
            case b_function:
            {
                object f;
                function_init(&f);
                f.fp->enclosing_scope=malloc(sizeof(scope));
                memcpy(f.fp->enclosing_scope, &scope, sizeof(scope));
                f.fp->enviroment=(void*)environment;
                reference(&scope);// remember to check the enclosing scope in destructor
                object arguments_count_object=pop(object_stack);
                if(arguments_count_object.type!=t_number){
                    ERROR(WRONG_ARGUMENT_TYPE, "Number of function arguments isn't present or has a wrong type, number of instruction is: %i\n", *pointer);
                    break;
                }
                f.fp->arguments_count=(int)arguments_count_object.value;
                f.fp->ftype=f_bytecode;
                f.fp->label=instr.argument;
                push(object_stack, f);
                break;
            }
            case b_call:
            {
                object o=pop(object_stack);
                int arguments_count=instr.argument;
                if(o.type==t_null){
                    ERROR(WRONG_ARGUMENT_TYPE, "Called function is null.");
                }
                if(o.type!=t_function){
                    object* arguments=malloc(sizeof(object)*arguments_count);
                    for (int i = 0; i < arguments_count; i++){
                        arguments[i]=pop(object_stack);
                    }
                    push(object_stack, call(o, arguments, arguments_count));
                    free(arguments);
                }
                object f=cast(o, t_function);
                if(f.fp->ftype==f_native){
                    object* arguments=malloc(sizeof(object)*arguments_count);
                    // items are on stack in reverse order, but native function expect them to be in normal order
                    for (int i = arguments_count-1; i >= 0; i--){
                        arguments[i]=pop(object_stack);
                    }
                    push(object_stack, f.fp->pointer(arguments, arguments_count));
                    free(arguments);
                } else if(f.fp->ftype==f_bytecode){
                    object function_scope;
                    table_init(&function_scope);
                    if(f.fp->enclosing_scope!=NULL){
                        inherit_scope(function_scope, *f.fp->enclosing_scope);
                    } else {
                        inherit_scope(function_scope, scope);
                    }
                    int destination=search_for_label(code, f.fp->label);
                    if(destination>0){
                        object return_point;
                        number_init(&return_point);
                        return_point.value=*pointer;
                        set(function_scope, "return_point", return_point);
                        push(return_stack, scope);
                        
                        scope=function_scope;
                        *pointer=destination;
                    } else {
                        ERROR(WRONG_ARGUMENT_TYPE, "Incorrect function jump label %i, number of instruction is: %i\n", f.fp->label, *pointer);
                    }
               } else if(f.fp->ftype==f_ast) {
                   ERROR(NOT_IMPLEMENTED, "Can't call ast function from bytecode, number of instruction is: %i\n", *pointer);
               } else {
                    ERROR(INCORRECT_OBJECT_POINTER, "Incorrect function pointer, number of instruction is: %i\n", *pointer);
               }
               break;
            }
            case b_return:
            {
                if(return_stack->top==0){
                    return pop(object_stack);
                } else {
                    object return_point=get(scope, "return_point");
                    if(return_point.type!=t_number){
                        USING_STRING(stringify(return_point), 
                            ERROR(WRONG_ARGUMENT_TYPE, "Return point (%s) is not a number, number of instruction is: %i\n", str, *pointer));
                    } else {
                        *pointer=return_point.value;
                        object return_scope=pop(return_stack);
                        if(return_scope.type!=t_table){
                            USING_STRING(stringify(return_scope), 
                                ERROR(WRONG_ARGUMENT_TYPE, "Return scope (%s) is not a table, number of instruction is: %i\n", str, *pointer));
                        } else {
                            object_deinit(&scope);
                            scope=return_scope;     
                        }
                    }
                }
                break;
            }
            case b_label:
                break;
            default:
                ERROR(WRONG_ARGUMENT_TYPE, "Uncatched bytecode instruction type: %i, number of instruction is: %i\n", instr.type, *pointer);
        }
        (*pointer)++;
    }
    return pop(object_stack);
}