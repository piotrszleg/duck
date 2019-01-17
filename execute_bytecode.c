#include "execute_bytecode.h"

void push(stack* stack, object o){
    reference(&o);
    stack_push(stack, (const void*)(&o));
}

object pop(stack* stack){
    void* pop_result=stack_pop(stack);
    object* o=pop_result;
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

#define INITIAL_LABELS_COUNT 4
int* list_labels(instruction* code){
    int* labels=malloc(INITIAL_LABELS_COUNT*sizeof(int));
    int labels_count=INITIAL_LABELS_COUNT;
    int pointer=0;
    while(code[pointer].type!=b_end){
        if(code[pointer].type==b_label){
            while(code[pointer].argument>labels_count){
                labels_count*=2;
                labels=realloc(labels, labels_count*sizeof(int));
            }
            labels[code[pointer].argument]=pointer;
        }
        pointer++;
    }
    return labels;
}

void move_to_function(bytecode_environment* environment, function* f, bool termainate){
    // create and push return_point pointing to current location
    return_point rp;
    rp.program=environment->program;
    rp.pointer=environment->pointer;
    rp.scope=environment->scope;
    rp.terminate=termainate;
    stack_push(&environment->return_stack, &rp);

    object function_scope;
    table_init(&function_scope);
    if(f->enclosing_scope.type!=t_null){
        inherit_scope(function_scope, f->enclosing_scope);
    }
    environment->scope=function_scope;
    environment->program=f->source_pointer;
    environment->pointer=0;
}

void bytecode_enviroment_init(bytecode_environment* e){
    e->labels=list_labels(e->program->code);
    stack_init(&e->object_stack, sizeof(object), STACK_SIZE);
    push(&e->object_stack, null_const);
    stack_init(&e->return_stack, sizeof(return_point), STACK_SIZE);
}

object execute_bytecode(bytecode_environment* environment){
    int* pointer=&environment->pointer;// points to the current instruction

    while(true){
        bytecode_program* program=environment->program;
        stack* object_stack=&environment->object_stack;
        stack* return_stack=&environment->return_stack;
        instruction* code=program->code;
        void* constants=program->constants;

        instruction instr=code[*pointer];
        exec_state.line=program->information[*pointer].line;
        exec_state.column=program->information[*pointer].column;
        switch(instr.type){
            case b_discard:
            {
                // remove item from the stack and delete it if it's not referenced
                object top=pop(object_stack);
                dereference(&top);
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
                    indexed=environment->scope;
                }
                USING_STRING(stringify(key), 
                        push(object_stack, get(indexed, str)));
                if(instr.argument){
                    // delete indexed if it isn't the scope
                    dereference(&indexed);
                }
                dereference(&key);
                break;
            }
            case b_set:
            {
                object key=pop(object_stack);
                object indexed;
                if(instr.argument){
                    indexed=pop(object_stack);
                } else {
                    indexed=environment->scope;
                }
                object value=pop(object_stack);
                set(indexed, stringify(key), value);
                if(instr.argument){
                    // delete indexed if it isn't the scope
                    dereference(&indexed);
                }
                dereference(&key);
                push(object_stack, value);
                dereference(&value);
                break;
            }
            case b_get_scope:
            {
                push(object_stack, environment->scope);
                break;
            }
            case b_set_scope:
            {
                dereference(&environment->scope);
                object o=pop(object_stack);
                if(o.type!=t_table){
                    USING_STRING(stringify(o),
                        ERROR(WRONG_ARGUMENT_TYPE, "b_set_scope: %s isn't a table, number of instruction is: %i\n", str, *pointer));
                } else {
                    environment->scope=o;
                }
                break;
            }
            case b_discard_scope:
                ERROR(NOT_IMPLEMENTED, "With current scope handling discard scope doesn't work.");
            case b_new_scope:
            {
                object t;
                table_init(&t);
                dereference(&environment->scope);
                environment->scope=t;
                break;
            }
            case b_unary:
            {
                object op=pop(object_stack);
                object a=pop(object_stack);
                object b=pop(object_stack); 
                push(object_stack, operator(a, b, stringify(op)));
                dereference(&op);
                dereference(&a);
                dereference(&b);
                break;
            }
            case b_prefix:
            {
                object op=pop(object_stack);
                object a=pop(object_stack);
                push(object_stack, operator(a, null_const, stringify(op)));
                dereference(&op);
                dereference(&a);
                break;
            }
            case b_jump_not:
            {
                object condition=pop(object_stack);
                if(!is_falsy(condition)){
                    dereference(&condition);
                    break;// go to the next line
                }
                dereference(&condition);
                // else go to case label underneath
            }
            case b_jump:
            {
                int destination=environment->labels[instr.argument];
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
                f.fp->enviroment=environment;
                f.fp->enclosing_scope=environment->scope;
                reference(&environment->scope);// remember to check the enclosing scope in destructor
                object arguments_count_object=pop(object_stack);
                if(arguments_count_object.type!=t_number){
                    ERROR(WRONG_ARGUMENT_TYPE, "Number of function arguments isn't present or has a wrong type, number of instruction is: %i\n", *pointer);
                    break;
                }
                f.fp->arguments_count=(int)arguments_count_object.value;
                f.fp->ftype=f_bytecode;
                f.fp->source_pointer=environment->program->sub_programs+instr.argument;
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
                    break;
                }
                else if(o.fp->ftype==f_native){
                    object* arguments=malloc(sizeof(object)*arguments_count);
                    // items are on stack in reverse order, but native function expect them to be in normal order
                    for (int i = arguments_count-1; i >= 0; i--){
                        arguments[i]=pop(object_stack);
                    }
                    push(object_stack, o.fp->native_pointer(arguments, arguments_count));
                    free(arguments);
                } else if(o.fp->ftype==f_bytecode){
                    move_to_function(environment, o.fp, false);
                    continue;// don't increment the pointer
               } else if(o.fp->ftype==f_ast) {
                   ERROR(NOT_IMPLEMENTED, "Can't call ast function from bytecode, number of instruction is: %i\n", *pointer);
               } else {
                    ERROR(INCORRECT_OBJECT_POINTER, "Incorrect function pointer, number of instruction is: %i\n", *pointer);
               }
               break;
            }
            case b_end:
            case b_return:
            {
                if(return_stack->top==0){
                    return pop(object_stack);
                } else {
                    return_point* rp=stack_pop(return_stack);
                    if(rp->terminate){
                        object last=pop(object_stack);
                        reference(&last);
                        return last;
                    } else {
                        environment->program=rp->program;
                        environment->pointer=rp->pointer;
                        dereference(&environment->scope);
                        environment->scope=rp->scope;
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
}