#include "execute_bytecode.h"

#define STACK_SIZE 16

void push(vector* stack, Object o){
    reference(&o);
    vector_push(stack, (const void*)(&o));
}

Object pop(vector* stack){
    void* pop_result=vector_pop(stack);
    Object* o=pop_result;
    return *o;
}

Object peek(vector* stack){
    return *(Object*)vector_top(stack);
}

char* stringify_object_stack(Executor* E, const vector* s){
    int pointer=0;
    int string_end=0;
    int result_size=64;
    char* result=calloc(64, sizeof(char));
    CHECK_ALLOCATION(result);
    
    Object* casted_items=(Object*)s->items;
    while(pointer<vector_count(s)){
        char* stringified_item=stringify(E, *(casted_items+pointer));
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

void move_to_function(Executor* E, Function* f){
    gc_object_reference(f->source_pointer);

    Object function_scope;
    table_init(E, &function_scope);
    if(f->enclosing_scope.type!=t_null){
        inherit_scope(E, function_scope, f->enclosing_scope);
    }
    dereference(E, &E->bytecode_environment.scope);
    E->bytecode_environment.scope=function_scope;
    gc_object_dereference(E, (gc_Object*)E->bytecode_environment.executed_program);
    E->bytecode_environment.executed_program=f->source_pointer;
    E->bytecode_environment.pointer=0;
}

void create_return_point(BytecodeEnvironment* environment, bool terminate){
    ReturnPoint return_point;
    gc_object_reference((gc_Object*)environment->executed_program);
    return_point.program=environment->executed_program;
    return_point.pointer=environment->pointer;
    reference(&environment->scope);
    return_point.scope=environment->scope;
    return_point.terminate=terminate;
    vector_push(&environment->return_stack, &return_point);
}

void bytecode_environment_init(BytecodeEnvironment* environment){
    environment->pointer=0;
    vector_init(&environment->debugger.breakpoints, sizeof(Breakpoint*), 4);
    environment->debugger.running=false;
    vector_init(&environment->object_stack, sizeof(Object), STACK_SIZE);
    push(&environment->object_stack, null_const);
    vector_init(&environment->return_stack, sizeof(ReturnPoint), STACK_SIZE);
}

void bytecode_environment_free(BytecodeEnvironment* environment){
    for(int i=0; i<vector_count(&environment->debugger.breakpoints); i++){
        free(pointers_vector_get(&environment->debugger.breakpoints, i));
    }
    vector_deinit(&environment->debugger.breakpoints);
    vector_deinit(&environment->object_stack);
    vector_deinit(&environment->return_stack);
    
    free(environment);
}

Object evaluate_string(Executor* E, const char* s, Object scope);

void debugger(Executor* E){
    BytecodeEnvironment* environment=&E->bytecode_environment;
    if(environment->debugger.running){
        for(int i=0; i<vector_count(&environment->debugger.breakpoints); i++){
            Breakpoint* br=(Breakpoint*)pointers_vector_get(&environment->debugger.breakpoints, i);

            if(strcmp(E->file, br->file)==0
            && E->line==br->line) {
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
            get_execution_info(E, e_info, sizeof(e_info));
            printf("%s\n", e_info);
        )
        COMMAND("memory",
            print_allocated_objects(E);
        )
        COMMAND("stack",
            USING_STRING(stringify_object_stack(E, &environment->object_stack),
                printf("%s\n", str));
        )
        COMMAND("scope",
            USING_STRING(stringify(E, environment->scope),
                printf("%s\n", str));
        )
        COMMAND_PARAMETERIZED("eval", 
            Object result=evaluate_string(E, parameter, environment->scope);
            USING_STRING(stringify(E, result),
                printf("%s\n", str));
            dereference(E, &result);
            return;
        )
        COMMAND("breakpoints", 
            for(int i=0; i<vector_count(&environment->debugger.breakpoints); i++){
                Breakpoint* br=(Breakpoint*)pointers_vector_get(&environment->debugger.breakpoints, i);
                printf("%s:%i\n", br->file, br->line);
            }
        )
        COMMAND_PARAMETERIZED("break",
            Breakpoint* b=malloc(sizeof(Breakpoint));
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
            
            vector_push(&environment->debugger.breakpoints, &b);
            return;
        )
        COMMAND_PARAMETERIZED("remove",
            Breakpoint b;

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

            for(int i=0; i<vector_count(&environment->debugger.breakpoints); i++){
                Breakpoint* br=(Breakpoint*)pointers_vector_get(&environment->debugger.breakpoints, i);
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

Object execute_bytecode(Executor* E){
    int* pointer=&E->bytecode_environment.pointer;// points to the current Instruction

    while(true){
        if(E->options.debug_mode){
            debugger(E);
        }
        BytecodeProgram* program=E->bytecode_environment.executed_program;
        vector* object_stack=&E->bytecode_environment.object_stack;
        vector* return_stack=&E->bytecode_environment.return_stack;
        Instruction* code=program->code;
        void* constants=program->constants;
        Object* scope=&E->bytecode_environment.scope;

        Instruction instr=code[*pointer];
        E->line=program->information[*pointer].line;
        E->column=program->information[*pointer].column;

        #define BYTECODE_ERROR(cause, message, ...) \
            {   Object err; \
                NEW_ERROR(err, "BYTECODE_ERROR", cause, message ", number of instruction is %i", ##__VA_ARGS__, *pointer); \
                push(object_stack, err); \
                break;   }

        switch(instr.type){
            case b_discard:
            {
                // remove item from the stack and delete it if it's not referenced
                Object top=pop(object_stack);
                dereference(E, &top);
                break;
            }
            case b_no_op:
                break;
            #define INDEX_STACK(index) ((Object*)object_stack->items)[object_stack->count-1-(index)]
            case b_move_top:
            {
                for(int i=0; i<instr.uint_argument; i++){
                    Object temporary=INDEX_STACK(i);
                    INDEX_STACK(i)=INDEX_STACK(i+1);
                    INDEX_STACK(i+1)=temporary;
                }
                break;
            }
            case b_push_to_top:
            {
                for(int i=instr.uint_argument; i>=1; i--){
                    Object temporary=INDEX_STACK(i);
                    INDEX_STACK(i)=INDEX_STACK(i-1);
                    INDEX_STACK(i-1)=temporary;
                }
                break;
            }
            case b_swap:
            {
                Object temporary=INDEX_STACK(instr.swap_argument.left);
                INDEX_STACK(instr.swap_argument.left)=INDEX_STACK(instr.swap_argument.right);
                INDEX_STACK(instr.swap_argument.right)=temporary;
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
                Object s;
                string_init(&s);
                s.text=strdup(((char*)constants)+instr.uint_argument);
                push(object_stack, s);
                break;
            }
            case b_load_float:
            {
                push(object_stack, to_float(instr.float_argument));
                break;
            }
            case b_load_int:
            {
                push(object_stack, to_int(instr.int_argument));
                break;
            }
            case b_table_literal:
            {
                Object t;
                table_init(E, &t);
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
                Object key=pop(object_stack);
                push(object_stack, get(E, *scope, key));
                dereference(E, &key);
                break;
            }
            case b_table_get:
            {
                Object key=pop(object_stack);
                Object indexed=pop(object_stack);
                push(object_stack, get(E, indexed, key));
                dereference(E, &indexed);
                dereference(E, &key);
                break;
            }
            case b_set:
            {
                Object key=pop(object_stack);
                Object value=pop(object_stack);
                push(object_stack, set(E, *scope, key, value));
                dereference(E, &key);
                dereference(E, &value);
                break;
            }
            case b_table_set:
            {
                Object key=pop(object_stack);
                Object indexed=pop(object_stack);
                Object value=pop(object_stack);
                push(object_stack, set(E, indexed, key, value));
                dereference(E, &indexed);
                dereference(E, &key);
                dereference(E, &value);
                break;
            }
            case b_table_set_keep:
            {
                Object key=pop(object_stack);
                Object value=pop(object_stack);
                Object indexed=pop(object_stack);
                Object set_result=set(E, indexed, key, value);
                push(object_stack, indexed);
                dereference(E, &indexed);

                // make sure that set_result will only be deleted
                // if it is not the same Object as value
                destroy_unreferenced(E, &set_result);

                dereference(E, &key);
                dereference(E, &value);
                break;
            }
            case b_get_scope:
            {
                push(object_stack, *scope);
                break;
            }
            case b_set_scope:
            {
                dereference(E, scope);
                Object o=pop(object_stack);
                if(o.type!=t_table){
                    BYTECODE_ERROR(o, "b_set_scope: Object isn't a table. It's type is: %i", o.type);
                } else {
                    reference(&o);
                    *scope=o;
                }
                break;
            }
            case b_new_scope:
            {
                Object t;
                table_init(E, &t);
                inherit_scope(E, t, *scope);
                reference(&t);
                dereference(E, scope);
                *scope=t;
                break;
            }
            case b_binary:
            {
                Object op=pop(object_stack);
                Object a=pop(object_stack);
                Object b=pop(object_stack); 
                if(op.type!=t_string){
                    BYTECODE_ERROR(op, "Operator must be of string type, it's type is %s", OBJECT_TYPE_NAMES[op.type]);
                }
                push(object_stack, operator(E, a, b, op.text));
                dereference(E, &op);
                dereference(E, &a);
                dereference(E, &b);
                break;
            }
            case b_prefix:
            {
                Object op=pop(object_stack);
                Object a=pop(object_stack);
                if(op.type!=t_string){
                    BYTECODE_ERROR(op, "Operator must be of string type, it's type is %s", OBJECT_TYPE_NAMES[op.type]);
                }
                push(object_stack, operator(E, null_const, a, op.text));
                dereference(E, &op);
                dereference(E, &a);
                break;
            }
            case b_jump_not:
            {
                Object condition=pop(object_stack);
                if(!is_falsy(condition)){
                    dereference(E, &condition);
                    break;// go to the next line
                }
                dereference(E, &condition);
                // else go to case label underneath
            }
            case b_jump:
            {
                *pointer=E->bytecode_environment.executed_program->labels[instr.uint_argument];
                break;
            }
            // these two instructions should always be called one after another
            case b_pre_function:
            {
                Object f;
                function_init(E, &f);
                f.fp->enclosing_scope=*scope;
                reference(scope);// remember to check the enclosing scope in destructor
                f.fp->arguments_count=instr.pre_function_argument.arguments_count;
                f.fp->variadic=instr.pre_function_argument.is_variadic;
                f.fp->ftype=f_bytecode;
                push(object_stack, f);
                break;
            }
            case b_function:
            {
                Object f=peek(object_stack);
                if(f.type!=t_function){
                    BYTECODE_ERROR(f, "b_function: bytecode function type is %s", OBJECT_TYPE_NAMES[f.type]);
                }
                f.fp->source_pointer=E->bytecode_environment.executed_program->sub_programs+instr.uint_argument;
                gc_object_reference((gc_Object*)f.fp->source_pointer);
                break;
            }
            case b_tail_call:
            case b_call:
            {
                Object o=pop(object_stack);
                int provided_arguments=instr.uint_argument;

                #define POP_ARGUMENTS \
                    for (int i = 0; i < provided_arguments; i++){ \
                        Object argument=pop(object_stack); \
                            dereference(E, &argument); \
                    } \
                    dereference(E, &o);
                #define CALL_ERROR(message, ...) \
                {   Object err; \
                    POP_ARGUMENTS \
                    NEW_ERROR(err, "CALL_ERROR", o, message, ##__VA_ARGS__); \
                    push(object_stack, err); \
                    break;  }
                
                if(o.type==t_null){
                    CALL_ERROR("Called function is null.");
                }
                // if object isn't a function it can be called through using monkey patching or Table call field
                if(o.type!=t_function){
                    Object* arguments=malloc(sizeof(Object)*provided_arguments);
                    for (int i = 0; i < provided_arguments; i++){
                        arguments[i]=pop(object_stack);
                    }
                    push(object_stack, call(E, o, arguments, provided_arguments));
                    free(arguments);
                    break;
                }
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
                    Object* arguments=malloc(sizeof(Object)*provided_arguments);
                    // items are on stack in reverse order, but native function expect them to be in normal order
                    for (int i = provided_arguments-1; i >= 0; i--){
                        arguments[i]=pop(object_stack);
                    }
                    Object call_result=o.fp->native_pointer(E, arguments, provided_arguments);
                    push(object_stack, call_result);
                    free(arguments);
                } else if(o.fp->ftype==f_bytecode) {
                    if(o.fp->variadic){
                        // pack variadic arguments into a Table and push the Table back onto the stack
                        int variadic_arguments_count=arguments_count_difference+1;
                        Object variadic_table;
                        table_init(E, &variadic_table);
                        for(int i=variadic_arguments_count-1; i>=0; i--){
                            set(E, variadic_table, to_int(i), pop(object_stack));
                        }
                        push(object_stack, variadic_table);
                    }
                    if(instr.type!=b_tail_call){
                        create_return_point(&E->bytecode_environment, false);
                    }
                    move_to_function(E, o.fp);
                    dereference(E, &o);
                    continue;// don't increment the pointer
                } else if(o.fp->ftype==f_special){
                    switch(o.fp->special_index){
                        case 0:// coroutine yield
                        {
                            if(E->coroutine==NULL){
                                POP_ARGUMENTS
                                Object err;
                                NEW_ERROR(err, "COROUTINE_ERROR", o, "Yield from outisde of coroutine.");
                                push(object_stack, err);
                            }
                            // simply return the value from stack top
                            if(E->options.debug_mode){
                                debugger(E);
                            }
                            (*pointer)++;
                            if(provided_arguments==1){
                                return pop(object_stack);
                            } else if(provided_arguments==0){
                                return null_const;
                            } else {
                                POP_ARGUMENTS
                                RETURN_ERROR("COROUTINE_ERROR", null_const, "Incorrect number of arguments (%i) was passed to coroutine yield.", provided_arguments);
                            }
                        }
                        case 1:// debugger
                        {
                            POP_ARGUMENTS
                            if(E->options.debug_mode){
                                E->bytecode_environment.debugger.running=false;
                                debugger(E);
                            }
                            push(object_stack, null_const);
                            (*pointer)++;
                            continue;
                        }
                        default:
                            CALL_ERROR("Unknown special function of special_index %i.", o.fp->special_index)
                    }
                } else if(o.fp->ftype==f_ast) {
                    CALL_ERROR("Can't call ast function from bytecode.");
                } else {
                    CALL_ERROR("Incorrect function type %i", o.fp->ftype);
                }
                #undef POP_ARGUMENTS
                #undef CALL_ERROR
                dereference(E, &o);
                break;
            }
            case b_end:
            case b_return:
            {
                if(E->options.debug_mode){
                    debugger(E);
                }
                gc_object_dereference(E, (gc_Object*)E->bytecode_environment.executed_program);
                dereference(E, scope);
                if(vector_empty(return_stack)){
                    if(E->coroutine!=NULL){
                        E->coroutine->state=co_finished;
                    }
                    return pop(object_stack);
                } else {
                    ReturnPoint* return_point=vector_pop(return_stack);
                    
                    E->bytecode_environment.executed_program=return_point->program;
                    E->bytecode_environment.pointer=return_point->pointer;
                    *scope=return_point->scope;
                    if(return_point->terminate){
                        Object last=pop(object_stack);
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