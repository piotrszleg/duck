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

object peek(stack* stack){
    return *(object*)stack_top(stack);
}

char* stringify_object_stack(executor* Ex, const stack* s){
    int pointer=0;
    int string_end=0;
    int result_size=64;
    char* result=calloc(64, sizeof(char));
    CHECK_ALLOCATION(result);
    
    object* casted_items=(object*)s->items;
    while(pointer<s->top){
        char* stringified_item=stringify(Ex, *(casted_items+pointer));
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

void list_program_labels(bytecode_program* program){
    program->labels=list_labels(program->code);
    for(int i=0; i<program->sub_programs_count; i++){
        list_program_labels(&program->sub_programs[i]);
    }
}

void move_to_function(executor* Ex, function* f, bool termainate){
    // create and push return_point pointing to current location
    return_point rp;
    rp.program=Ex->bytecode_env.program;
    rp.pointer=Ex->bytecode_env.pointer;
    rp.scope=Ex->bytecode_env.scope;
    rp.terminate=termainate;
    stack_push(&Ex->bytecode_env.return_stack, &rp);

    object function_scope;
    table_init(&function_scope);
    if(f->enclosing_scope.type!=t_null){
        inherit_scope(Ex, function_scope, f->enclosing_scope);
    }
    Ex->bytecode_env.scope=function_scope;
    Ex->bytecode_env.program=f->source_pointer;
    Ex->bytecode_env.pointer=0;
}

void bytecode_environment_init(bytecode_environment* environment){
    list_program_labels(environment->program);
    
    vector_init(&environment->debugger.breakpoints);

    environment->debugger.running=false;
    stack_init(&environment->object_stack, sizeof(object), STACK_SIZE);
    push(&environment->object_stack, null_const);
    stack_init(&environment->return_stack, sizeof(return_point), STACK_SIZE);
}

void free_labels(bytecode_program* program){
    free(program->labels);
    for(int i=0; i<program->sub_programs_count; i++){
        free_labels(&program->sub_programs[i]);
    }
}

void bytecode_environment_free(bytecode_environment* environment){
    for(int i=0; i<vector_total(&environment->debugger.breakpoints); i++){
        free(vector_get(&environment->debugger.breakpoints, i));
    }
    vector_free(&environment->debugger.breakpoints);
    stack_deinit(&environment->object_stack);
    stack_deinit(&environment->return_stack);

    free_labels(environment->program);
    bytecode_program_free(environment->program);
    free(environment);
}

object evaluate_string(executor* Ex, const char* s, object scope);

void debugger(executor* Ex){
    bytecode_environment* environment=&Ex->bytecode_env;
    if(environment->debugger.running){
        for(int i=0; i<vector_total(&environment->debugger.breakpoints); i++){
            breakpoint* br=(breakpoint*)vector_get(&environment->debugger.breakpoints, i);

            if(strcmp(Ex->file, br->file)==0
            && Ex->line==br->line) {
                environment->debugger.running=false;
            }
        }
        if(environment->debugger.running){
            return;
        }
    }
    char input[128];
    while(true){
        printf(">>");
        if(fgets_no_newline(input, sizeof(input), stdin)==NULL){
            return;
        }
        #define COMMAND(name, body) if(strcmp(input, name)==0){body return;}
        #define COMMAND_PARAMETERIZED(name, body) \
            if(strstr(input, name)==input){ \
                char* parameter=input+sizeof(name); \
                body \
                return; \
            }
        COMMAND("next",)
        COMMAND("",)
        COMMAND("run", environment->debugger.running=true;)
        COMMAND("help",
            printf("available commands are: \nnext \nrun \nposition \nmemory \nstack \nscope " \
            "\neval <expression> \nbreakpoints \nbreak <file>:<line> \nremove <file>:<line>\n");
        )
        COMMAND("position",
            char e_info[128];
            get_execution_info(Ex, e_info, sizeof(e_info));
            printf("%s\n", e_info);
        )
        COMMAND("memory",
            print_allocated_objects();
        )
        COMMAND("stack",
            USING_STRING(stringify_object_stack(Ex, &environment->object_stack),
                printf("%s\n", str));
        )
        COMMAND("scope",
            USING_STRING(stringify(Ex, environment->scope),
                printf("%s\n", str));
        )
        COMMAND_PARAMETERIZED("eval", 
            object result=evaluate_string(Ex, parameter, environment->scope);
            USING_STRING(stringify(Ex, result),
                printf("%s\n", str));
            dereference(Ex, &result);
            return;
        )
        COMMAND("breakpoints", 
            for(int i=0; i<vector_total(&environment->debugger.breakpoints); i++){
                breakpoint* br=(breakpoint*)vector_get(&environment->debugger.breakpoints, i);
                printf("%s:%i\n", br->file, br->line);
            }
        )
        COMMAND_PARAMETERIZED("break",
            breakpoint* b=malloc(sizeof(breakpoint));
            CHECK_ALLOCATION(b)

            // parameter has syntax file:line_number
            int i=0;
            while(parameter[i]!=':' && parameter[i]!='\0') i++;
            if(parameter[i]=='\0'){
                printf("Wrong parameter given to break command: %s", parameter);
            }

            char* buf=malloc((i+1)*(sizeof(char)));
            memcpy(buf, parameter, i);
            buf[i]='\0';

            b->file=buf;
            b->line=atoi(parameter+i+1);
            
            vector_add(&environment->debugger.breakpoints, b);
            return;
        )
        COMMAND_PARAMETERIZED("remove",
            breakpoint b;

            int i=0;
            while(parameter[i]!=':' && parameter[i]!='\0') i++;
            if(parameter[i]=='\0'){
                printf("Wrong parameter given to remove command: %s", parameter);
            }

            char* buf=malloc((i+1)*(sizeof(char)));
            memcpy(buf, parameter, i);
            buf[i]='\0';

            b.file=buf;
            b.line=atoi(parameter+i+1);

            for(int i=0; i<vector_total(&environment->debugger.breakpoints); i++){
                breakpoint* br=(breakpoint*)vector_get(&environment->debugger.breakpoints, i);
                if(strcmp(br->file, b.file)==0 && br->line==b.line){
                    vector_delete(&environment->debugger.breakpoints, i);
                }
            }
            free(buf);
            return;
        )
        #undef COMMAND
        #undef COMMAND_PARAMETERIZED
        printf("Unknown command \"%s\"\n", input);
    }
}

object execute_bytecode(executor* Ex){
    int* pointer=&Ex->bytecode_env.pointer;// points to the current instruction

    while(true){
        if(Ex->opt.debug_mode){
            debugger(Ex);
        }
        bytecode_program* program=Ex->bytecode_env.program;
        stack* object_stack=&Ex->bytecode_env.object_stack;
        stack* return_stack=&Ex->bytecode_env.return_stack;
        instruction* code=program->code;
        void* constants=program->constants;
        object* scope=&Ex->bytecode_env.scope;

        instruction instr=code[*pointer];
        Ex->line=program->information[*pointer].line;
        Ex->column=program->information[*pointer].column;
        switch(instr.type){
            case b_discard:
            {
                // remove item from the stack and delete it if it's not referenced
                object top=pop(object_stack);
                dereference(Ex, &top);
                break;
            }
            case b_no_op:
                break;
            #define INDEX_STACK(index) ((object*)object_stack->items)[object_stack->top-1-(index)]
            case b_move_top:
            {
                for(int i=0; i<instr.argument; i++){
                    object temporary=INDEX_STACK(i);
                    INDEX_STACK(i)=INDEX_STACK(i+1);
                    INDEX_STACK(i+1)=temporary;
                }
                break;
            }
            case b_push_to_top:
            {
                for(int i=instr.argument; i>=1; i--){
                    object temporary=INDEX_STACK(i);
                    INDEX_STACK(i)=INDEX_STACK(i-1);
                    INDEX_STACK(i-1)=temporary;
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
                push(object_stack, get(Ex, *scope, key));
                dereference(Ex, &key);
                break;
            }
            case b_table_get:
            {
                object key=pop(object_stack);
                object indexed=pop(object_stack);
                push(object_stack, get(Ex, indexed, key));
                dereference(Ex, &indexed);
                dereference(Ex, &key);
                break;
            }
            case b_set:
            {
                object key=pop(object_stack);
                object value=pop(object_stack);
                push(object_stack, set(Ex, *scope, key, value));
                dereference(Ex, &key);
                dereference(Ex, &value);
                break;
            }
            case b_table_set:
            {
                object key=pop(object_stack);
                object indexed=pop(object_stack);
                object value=pop(object_stack);
                push(object_stack, set(Ex, indexed, key, value));
                dereference(Ex, &indexed);
                dereference(Ex, &key);
                dereference(Ex, &value);
                break;
            }
            case b_table_set_keep:
            {
                object key=pop(object_stack);
                object value=pop(object_stack);
                object indexed=pop(object_stack);
                object set_result=set(Ex, indexed, key, value);
                push(object_stack, indexed);
                dereference(Ex, &indexed);

                // make sure that set_result will only be deleted
                // if it is not the same object as value
                destroy_unreferenced(Ex, &set_result);

                dereference(Ex, &key);
                dereference(Ex, &value);
                break;
            }
            case b_get_scope:
            {
                push(object_stack, *scope);
                break;
            }
            case b_set_scope:
            {
                dereference(Ex, scope);
                object o=pop(object_stack);
                if(o.type!=t_table){
                    USING_STRING(stringify(Ex, o),
                        THROW_ERROR(WRONG_ARGUMENT_TYPE, "b_set_scope: %s isn't a table, number of instruction is: %i\n", str, *pointer));
                } else {
                    reference(&o);
                    *scope=o;
                }
                break;
            }
            case b_new_scope:
            {
                object t;
                table_init(&t);
                inherit_scope(Ex, t, *scope);
                reference(&t);
                dereference(Ex, scope);
                *scope=t;
                break;
            }
            case b_binary:
            {
                object op=pop(object_stack);
                object a=pop(object_stack);
                object b=pop(object_stack); 
                push(object_stack, operator(Ex, a, b, stringify(Ex, op)));
                dereference(Ex, &op);
                dereference(Ex, &a);
                dereference(Ex, &b);
                break;
            }
            case b_prefix:
            {
                object op=pop(object_stack);
                object a=pop(object_stack);
                push(object_stack, operator(Ex, a, null_const, stringify(Ex, op)));
                dereference(Ex, &op);
                dereference(Ex, &a);
                break;
            }
            case b_jump_not:
            {
                object condition=pop(object_stack);
                if(!is_falsy(condition)){
                    dereference(Ex, &condition);
                    break;// go to the next line
                }
                dereference(Ex, &condition);
                // else go to case label underneath
            }
            case b_jump:
            {
                *pointer=Ex->bytecode_env.program->labels[instr.argument];
                break;
            }
            case b_function:
            {
                object f;
                function_init(&f);

                f.fp->environment=NULL;

                f.fp->enclosing_scope=*scope;
                reference(scope);// remember to check the enclosing scope in destructor
                object arguments_count_object=pop(object_stack);
                if(arguments_count_object.type!=t_number){
                    THROW_ERROR(WRONG_ARGUMENT_TYPE, "Number of function arguments isn't present or has a wrong type, number of instruction is: %i\n", *pointer);
                    break;
                }
                f.fp->arguments_count=(int)arguments_count_object.value;

                object variadic_object=pop(object_stack);
                f.fp->variadic=!is_falsy(variadic_object);

                dereference(Ex, &variadic_object);
                dereference(Ex, &arguments_count_object);
                f.fp->ftype=f_bytecode;
                f.fp->source_pointer=Ex->bytecode_env.program->sub_programs+instr.argument;
                push(object_stack, f);
                break;
            }
            case b_call:
            {
                object o=pop(object_stack);
                int provided_arguments=instr.argument;

                #define CALL_ERROR(message, ...) \
                    object err; \
                    NEW_ERROR(err, "CALL_ERROR", o, message, ##__VA_ARGS__); \
                    push(object_stack, err); \
                    break;
                
                if(o.type==t_null){
                    CALL_ERROR("Called function is null.");
                }
                // provided object isn't a function but it can be called through using monkey patching or table call field
                if(o.type!=t_function){
                    object* arguments=malloc(sizeof(object)*provided_arguments);
                    for (int i = 0; i < provided_arguments; i++){
                        arguments[i]=pop(object_stack);
                    }
                    push(object_stack, call(Ex, o, arguments, provided_arguments));
                    free(arguments);
                    break;
                }
                /*
                if(o.fp->ftype=f_special){
                    switch(o.fp->special_index){
                        case 0:// coroutine
                        {
                            bytecode_environment* new_environment=malloc(sizeof(bytecode_environment));
                            new_environment->pointer=0;
                            new_environment->program=malloc(sizeof(bytecode_program));
                            object subscope;
                            table_init(&subscope);
                            inherit_scope(subscope, environment->scope);
                            object gcp;
                            gcp.gcp=(gc_pointer*)new_environment;
                            gcp.gcp->destructor=(gc_pointer_destructor)bytecode_environment_free;
                            gc_pointer_init(&gcp);

                            // create coroutine object
                            // set it's owner environment to this environment
                        }
                        case 1:// yield
                        {
                            // push the value from stack into the owner's stack
                            // return control to the owner
                        }
                        case 2:// coroutine call
                        {
                            // get coroutine from the stack
                            // push the value from stack into the coroutine's stack
                            // return control to the coroutine
                        }
                    }
                }*/
                int arguments_count_difference=provided_arguments-o.fp->arguments_count;
                // check arguments count
                if(o.fp->variadic){
                    // variadic function can be called with no variadic arguments
                    if(arguments_count_difference<-1){
                        CALL_ERROR("Not enough arguments in variadic function call, expected at least %i, given %i.", o.fp->arguments_count-1, provided_arguments);
                    }
                } else {
                    if(arguments_count_difference>0) {
                        CALL_ERROR("Too many arguments in function call, expected %i, given %i.", o.fp->arguments_count, provided_arguments);
                    } else if(arguments_count_difference<0){
                        CALL_ERROR("Not enough arguments in function call, expected %i, given %i.", o.fp->arguments_count, provided_arguments);
                    }
                }

                if(o.fp->ftype==f_native){
                    object* arguments=malloc(sizeof(object)*provided_arguments);
                    // items are on stack in reverse order, but native function expect them to be in normal order
                    for (int i = provided_arguments-1; i >= 0; i--){
                        arguments[i]=pop(object_stack);
                    }
                    object call_result=o.fp->native_pointer(Ex, arguments, provided_arguments);
                    push(object_stack, call_result);
                    free(arguments);
                } else{
                    if(o.fp->variadic){
                        if(arguments_count_difference<-1){
                            CALL_ERROR("Not enough arguments in variadic function call, expected at least %i, given %i.", o.fp->arguments_count-1, provided_arguments);
                        }
                        // pack variadic arguments into a table and push the table back onto the stack
                        int variadic_arguments_count=arguments_count_difference+1;
                        object variadic_table;
                        table_init(&variadic_table);
                        for(int i=variadic_arguments_count-1; i>=0; i--){
                            set(Ex, variadic_table, to_number(i), pop(object_stack));
                        }
                        push(object_stack, variadic_table);
                    }
                    if(o.fp->ftype==f_bytecode){
                        move_to_function(Ex, o.fp, false);
                        continue;// don't increment the pointer
                    } else if(o.fp->ftype==f_ast) {
                        CALL_ERROR("Can't call ast function from bytecode.");
                    } else {
                        THROW_ERROR(INCORRECT_OBJECT_POINTER, "Incorrect function pointer, number of instruction is: %i\n", *pointer);
                    }
                }
                #undef CALL_ERROR
                break;
            }
            case b_end:
            case b_return:
            {
                if(Ex->opt.debug_mode){
                    debugger(Ex);
                }
                if(return_stack->top==0){
                    return pop(object_stack);
                } else {
                    return_point* rp=stack_pop(return_stack);
                    Ex->bytecode_env.program=rp->program;
                    Ex->bytecode_env.pointer=rp->pointer;
                    dereference(Ex, scope);
                    *scope=rp->scope;
                    if(rp->terminate){
                        object last=pop(object_stack);
                        return last;
                    }
                }
                break;
            }
            case b_label:
                break;
            default:
                THROW_ERROR(WRONG_ARGUMENT_TYPE, "Uncatched bytecode instruction type: %i, number of instruction is: %i\n", instr.type, *pointer);
        }
        (*pointer)++;
    }
}