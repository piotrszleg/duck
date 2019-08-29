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

void print_assumption(Executor* E, Assumption* a){
    switch(a->assumption_type){
        case a_constant:
            USING_STRING(stringify(E, a->constant),
                printf("constant(%s)", str))
            break;
        case a_type:
            printf("type(%s)", get_type_name(a->type));
            break;
        default:
            printf("Incorrect assumption type.");
    }
}

typedef enum {
    ac_not_equal=0,
    ac_equal=1,
    ac_compatible=2,
} AssumptionCompareResult;

AssumptionCompareResult compare_assumptions(Executor* E, Assumption* a, Assumption* b){
    if(a->assumption_type==a_constant&&b->assumption_type==a_constant){
        return compare(E, a->constant, b->constant)==0;
    } else if(a->assumption_type==a_type&&b->assumption_type==a_type){
        return a->type==b->type;
    } else {
        return a->type==b->type ? ac_compatible : 0;
    }
}

void assumption_deinit(Executor* E, Assumption* assumption){
    if(assumption->assumption_type==a_constant){
        dereference(E, &assumption->constant);
    }
}

void statistics_init(CallStatistics* statistics, uint call_fields, uint collected_calls) {
    statistics->last_call=calloc(call_fields, sizeof(Object));
    statistics->previous_calls=malloc(collected_calls*sizeof(Assumption*));
    statistics->constant_streaks=calloc(call_fields, sizeof(uint));
    statistics->collected_calls=0;
    statistics->initialized=true;
}

void statistics_deinit(CallStatistics* statistics) {
    statistics->initialized=false;
    free(statistics->previous_calls);
    free(statistics->last_call);
    free(statistics->constant_streaks);
}

void create_variant(Executor* E, BytecodeProgram* program) {
    CallStatistics* statistics=&program->statistics;
    if(!statistics->initialized){
        return;
    }
    int arguments_count=program->expected_arguments;
    int call_fields=arguments_count+program->upvalues_count;

    bool* already_covered=calloc(statistics->collected_calls, sizeof(bool));
    struct {
        unsigned index;
        unsigned score;
    } most_compatible_signature={0, 0};
    for(int i=0; i<statistics->collected_calls; i++){
        if(!already_covered[i]){
            unsigned score=0;
            for(int j=0; j<statistics->collected_calls; j++){
                bool exactly_equal=true;
                for(int a=0; a<call_fields; a++){
                    switch(compare_assumptions(E, &statistics->previous_calls[i][a], &statistics->previous_calls[j][a])){
                        case ac_equal:
                            score+=2;
                            break;
                        case ac_compatible:
                            score++;
                            exactly_equal=false;
                            break;
                        case ac_not_equal:
                            goto next;
                        default:;
                    }
                }
                if(exactly_equal){
                    already_covered[j]=true;
                }
                next:;
            }
            if(score>most_compatible_signature.score){
                most_compatible_signature.score=score;
                most_compatible_signature.index=i;
            }
        }
    }
    if(E->options.print_bytecode_optimisations){
        printf("Collected the following data:\n");
        for(int i=0; i<statistics->collected_calls; i++){
            printf("[");
            for(int j=0; j<call_fields; j++){
                print_assumption(E, &statistics->previous_calls[i][j]);
                printf(", ");
            }
            printf("]\n");
        }
        printf("Generating variant with assumptions:\n");
        printf("[");
        for(int i=0; i<call_fields; i++){
            print_assumption(E, &statistics->previous_calls[most_compatible_signature.index][i]);
            printf(", ");
        }
        printf("]\n");
    }
    BytecodeProgram* variant=bytecode_program_copy(E, program, false);
    variant->assumptions=statistics->previous_calls[most_compatible_signature.index];
    optimise_bytecode(E, variant, E->options.print_bytecode_optimisations);
    pointers_vector_push(&program->variants, variant);

    for(int i=0; i<statistics->collected_calls; i++){
        if(i!=most_compatible_signature.index){
            for(int j=0; j<call_fields; j++){
                assumption_deinit(E, &statistics->previous_calls[i][j]);
            }
            free(statistics->previous_calls[i]);
        }
    }
    statistics_deinit(&program->statistics);
    program->calls_count=0;
}

void proccess_statistics(Executor* E, BytecodeEnvironment* environment){
    BytecodeProgram* program=environment->executed_program;
    program->calls_count++;
    if(!E->options.optimise_at_runtime||program->calls_count<=E->options.calls_before_optimisation){
        return;
    }
    int arguments_count=program->expected_arguments;
    int call_fields=arguments_count+program->upvalues_count;
    CallStatistics* statistics=&program->statistics;
    if(!statistics->initialized){
        if(E->options.print_bytecode_optimisations){
            printf("<BEGAN RECORDING>");
        }
        statistics_init(&program->statistics, call_fields, E->options.collected_calls);
    }
    Assumption* current_call=malloc(sizeof(Assumption)*call_fields);
    for(int i=0; i<call_fields; i++){
        Object argument;
        if(i<arguments_count){
            argument=*(Object*)vector_index(&environment->object_stack, vector_count(&environment->object_stack)-1-i);
        } else {
            char* upvalue_name=program->constants+program->upvalues[i-arguments_count];
            argument=get(E, E->scope, to_string(upvalue_name));
        }
        if(compare(E, statistics->last_call[i], argument)==0) {
            statistics->constant_streaks[i]++;
        } else {
            statistics->constant_streaks[i]=0;
        }
        if(statistics->constant_streaks[i]>=E->options.constant_threshold) {
            current_call[i].assumption_type=a_constant;
            current_call[i].constant=copy(E, argument);
            reference(&current_call[i].constant);
        } else {
            current_call[i].assumption_type=a_type;
            current_call[i].type=argument.type;
        }
        statistics->last_call[i]=argument;
        reference(&statistics->last_call[i]);
    }
    statistics->previous_calls[statistics->collected_calls]=current_call;
    statistics->collected_calls++;
    program->calls_count++;
    if(statistics->collected_calls>=E->options.collected_calls){
        create_variant(E, program);
    }
}

BytecodeProgram* choose_variant(Executor* E, BytecodeProgram* root){
    for(int i=0; i<vector_count(&root->variants); i++){
        BytecodeProgram* variant=pointers_vector_get(&root->variants, i);
        unsigned arguments_count=variant->expected_arguments;
        for(int j=0; j<arguments_count; i++){
            Object argument;
            if(j<arguments_count){
                argument=*(Object*)vector_index(&E->bytecode_environment.object_stack, vector_count(&E->bytecode_environment.object_stack)-1-i);
            } else {
                char* upvalue_name=variant->constants+variant->upvalues[i-arguments_count];
                argument=get(E, E->scope, to_string(upvalue_name));
            }
            Assumption a={a_constant, .constant=argument};
            if(compare_assumptions(E, &a, &variant->assumptions[j])==ac_not_equal){
                goto next_variant;
            }
        }
        return choose_variant(E, variant);
        next_variant:;
    }
    return root;
}

void move_to_function(Executor* E, Function* f){
    heap_object_reference(f->source_pointer);

    Object function_scope;
    table_init(E, &function_scope);
    if(f->enclosing_scope.type!=t_null){
        inherit_scope(E, function_scope, f->enclosing_scope);
    }
    reference(&function_scope);
    E->scope=function_scope;
    heap_object_reference((HeapObject*)f->source_pointer);

    E->bytecode_environment.executed_program=(BytecodeProgram*)f->source_pointer;
    E->bytecode_environment.pointer=0;
    
    proccess_statistics(E, &E->bytecode_environment);
}

void create_return_point(Executor* E, bool terminate){
    BytecodeEnvironment* environment=&E->bytecode_environment;
    
    ReturnPoint return_point;
    if(environment->executed_program!=NULL){
        heap_object_reference((HeapObject*)environment->executed_program);
    }
    return_point.program=environment->executed_program;
    return_point.pointer=environment->pointer;
    reference(&E->scope);
    return_point.scope=E->scope;
    return_point.terminate=terminate;
    vector_push(&environment->return_stack, &return_point);
}

// returns true if execute_bytecode function should return value from stack instead of continueing
bool pop_return_point(Executor* E) {
    if(E->bytecode_environment.executed_program!=NULL){
        heap_object_dereference(E, (HeapObject*)E->bytecode_environment.executed_program);
    }
    dereference(E, &E->scope);
    vector* return_stack=&E->bytecode_environment.return_stack;
    if(vector_empty(return_stack)){
        return true;
    }
    ReturnPoint* return_point=vector_pop(return_stack);
    E->bytecode_environment.executed_program=return_point->program;
    E->bytecode_environment.pointer=return_point->pointer;
    E->scope=return_point->scope;
    return return_point->terminate;
}

void bytecode_environment_init(BytecodeEnvironment* environment){
    environment->pointer=0;
    environment->executed_program=NULL;
    vector_init(&environment->debugger.breakpoints, sizeof(Breakpoint*), 4);
    environment->debugger.running=false;
    vector_init(&environment->object_stack, sizeof(Object), STACK_SIZE);
    push(&environment->object_stack, null_const);
    vector_init(&environment->return_stack, sizeof(ReturnPoint), STACK_SIZE);
}

void bytecode_environment_deinit(BytecodeEnvironment* environment){
    for(int i=0; i<vector_count(&environment->debugger.breakpoints); i++){
        free(pointers_vector_get(&environment->debugger.breakpoints, i));
    }
    vector_deinit(&environment->debugger.breakpoints);
    vector_deinit(&environment->object_stack);
    vector_deinit(&environment->return_stack);
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
            USING_STRING(stringify(E, E->scope),
                printf("%s\n", str));
        )
        COMMAND_PARAMETERIZED("eval", 
            Object result=evaluate_string(E, parameter, E->scope);
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
        if(E->options.debug){
            debugger(E);
        }
        BytecodeProgram* program=E->bytecode_environment.executed_program;
        vector* object_stack=&E->bytecode_environment.object_stack;
        Instruction instruction=program->code[*pointer];
        E->file=program->source_file_name;
        E->line=program->information[*pointer].line;
        E->column=program->information[*pointer].column;

        #define NEXT_INSTRUCTION \
            (*pointer)++; \
            Instruction instruction=program->code[*pointer];

        #define BYTECODE_ERROR(cause, message, ...) \
            {   Object err; \
                NEW_ERROR(err, "BYTECODE_ERROR", cause, message ", number of instruction is %i", ##__VA_ARGS__, *pointer); \
                push(object_stack, err); \
                break;   }

        switch(instruction.type){
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
                for(int i=0; i<instruction.uint_argument; i++){
                    Object temporary=INDEX_STACK(i);
                    INDEX_STACK(i)=INDEX_STACK(i+1);
                    INDEX_STACK(i+1)=temporary;
                }
                break;
            }
            case b_push_to_top:
            {
                for(int i=instruction.uint_argument; i>=1; i--){
                    Object temporary=INDEX_STACK(i);
                    INDEX_STACK(i)=INDEX_STACK(i-1);
                    INDEX_STACK(i-1)=temporary;
                }
                break;
            }
            case b_swap:
            {
                Object temporary=INDEX_STACK(instruction.swap_argument.left);
                INDEX_STACK(instruction.swap_argument.left)=INDEX_STACK(instruction.swap_argument.right);
                INDEX_STACK(instruction.swap_argument.right)=temporary;
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
                push(object_stack, to_string(((char*)program->constants)+instruction.uint_argument));
                break;
            }
            case b_load_float:
            {
                push(object_stack, to_float(instruction.float_argument));
                break;
            }
            case b_load_int:
            {
                push(object_stack, to_int(instruction.int_argument));
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
                push(object_stack, get(E, E->scope, key));
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
                push(object_stack, set(E, E->scope, key, value));
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
            case b_enter_scope:
            {
                Object new_scope;
                table_init(E, &new_scope);
                inherit_scope(E, new_scope, E->scope);

                push(object_stack, E->scope);

                reference(&new_scope);
                dereference(E, &E->scope);
                E->scope=new_scope;
                break;
            }
            case b_leave_scope:
            {
                dereference(E, &E->scope);
                Object o=pop(object_stack);
                if(o.type!=t_table){
                    BYTECODE_ERROR(o, "b_leave_scope: Object on stack isn't a table. It's type is: %i", o.type);
                } else {
                    reference(&o);
                    E->scope=o;
                }
                break;
            }
            case b_binary:
            {
                Object op=pop(object_stack);
                Object a=pop(object_stack);
                Object b=pop(object_stack); 
                if(op.type!=t_string){
                    BYTECODE_ERROR(op, "Operator must be of string type, it's type is %s", get_type_name(op.type));
                }
                push(object_stack, operator(E, a, b, op.text));
                dereference(E, &op);
                dereference(E, &a);
                dereference(E, &b);
                break;
            }
            #define OPERATOR_START \
                Object a=pop(object_stack); \
                Object b=pop(object_stack);
            #define OPERATOR_RESULT(result) \
                push(object_stack, result);
            #define OPERATOR_END \
                dereference(E, &a); \
                dereference(E, &b); \
                break;
            #define OPERATOR_FALLBACK(op) \
                push(object_stack, operator(E, a, b, op));
            case b_add:
            {
                OPERATOR_START
                if(a.type==t_int && b.type==t_int){
                    OPERATOR_RESULT(to_int(a.int_value+b.int_value));
                } else if(a.type==t_float && b.type==t_float){
                    OPERATOR_RESULT(to_float(a.float_value+b.float_value));
                } else if(a.type==t_string){
                    if(b.type!=t_string) {
                        USING_STRING(stringify(E, b),
                            OPERATOR_RESULT(to_string(string_add(a.text, str))))
                    } else {
                        OPERATOR_RESULT(to_string(string_add(a.text, b.text)));
                    }
                } else {
                    OPERATOR_FALLBACK("+")
                }
                OPERATOR_END
            }
            case b_add_string:
            {
                OPERATOR_START
                OPERATOR_RESULT(to_string(string_add(a.text, b.text)));
                OPERATOR_END
            }
            case b_add_float:
            {
                OPERATOR_START
                OPERATOR_RESULT(to_float(a.float_value+b.float_value));
                OPERATOR_END
            }
            case b_add_int:
            {
                OPERATOR_START
                OPERATOR_RESULT(to_int(a.int_value+b.int_value));
                OPERATOR_END
            }
            case b_multiply:
            {
                OPERATOR_START
                if(a.type==t_int && b.type==t_int){
                    OPERATOR_RESULT(to_int(a.int_value*b.int_value));
                } else if(a.type==t_float && b.type==t_float){
                    OPERATOR_RESULT(to_float(a.float_value*b.float_value));
                } else if(a.type==t_string && b.type==t_int && b.int_value>0){
                    OPERATOR_RESULT(to_string(string_repeat(a.text, b.int_value)));
                } else {
                    OPERATOR_FALLBACK("*")
                }
                OPERATOR_END
            }
            case b_multiply_int:
            {
                OPERATOR_START
                OPERATOR_RESULT(to_int(a.int_value*b.int_value));
                OPERATOR_END
            }
            case b_multiply_float:
            {
                OPERATOR_START
                OPERATOR_RESULT(to_float(a.float_value*b.float_value));
                OPERATOR_END
            }
            case b_divide:
            {
                OPERATOR_START
                if(a.type==t_int && b.type==t_int){
                    OPERATOR_RESULT(to_float((float)a.int_value/b.int_value));
                } else if(a.type==t_float && b.type==t_float){
                    OPERATOR_RESULT(to_float(a.float_value/b.float_value));
                } else {
                    OPERATOR_FALLBACK("/")
                }
                OPERATOR_END
            }
            case b_divide_float:
            {
                OPERATOR_START
                OPERATOR_RESULT(to_float(a.float_value/b.float_value));
                OPERATOR_END
            }
            case b_divide_int:
            {
                OPERATOR_START
                OPERATOR_RESULT(to_float((float)a.int_value/b.int_value));
                OPERATOR_END
            }
            case b_divide_floor:
            {
                OPERATOR_START
                if(a.type==t_int && b.type==t_int){
                    OPERATOR_RESULT(to_int(a.int_value/b.int_value));
                } else {
                    OPERATOR_FALLBACK("//")
                }
                OPERATOR_END
            }
            case b_divide_floor_int:
            {
                OPERATOR_START
                OPERATOR_RESULT(to_int(a.int_value/b.int_value));
                OPERATOR_END
            }
            case b_modulo:
            {
                OPERATOR_START
                if(a.type==t_int && b.type==t_int){
                    OPERATOR_RESULT(to_int(a.int_value%b.int_value));
                } else {
                    OPERATOR_FALLBACK("%")
                }
                OPERATOR_END
            }
            case b_modulo_int:
            {
                OPERATOR_START
                OPERATOR_RESULT(to_int(a.int_value%b.int_value));
                OPERATOR_END
            }
            case b_subtract:
            {
                OPERATOR_START
                if(a.type==t_int && b.type==t_int){
                    OPERATOR_RESULT(to_int(a.int_value-b.int_value));
                } else if(a.type==t_float && b.type==t_float){
                    OPERATOR_RESULT(to_float(a.float_value-b.float_value));
                } else {
                    OPERATOR_FALLBACK("-")
                }
                OPERATOR_END
            }
            case b_subtract_float:
            {
                OPERATOR_START
                OPERATOR_RESULT(to_float(a.float_value-b.float_value));
                OPERATOR_END
            }
            case b_subtract_int:
            {
                OPERATOR_START
                OPERATOR_RESULT(to_int(a.int_value-b.int_value));
                OPERATOR_END
            }
            case b_prefix:
            {
                Object op=pop(object_stack);
                Object a=pop(object_stack);
                if(op.type!=t_string){
                    BYTECODE_ERROR(op, "Operator must be of string type, it's type is %s", get_type_name(op.type));
                }
                push(object_stack, operator(E, null_const, a, op.text));
                dereference(E, &op);
                dereference(E, &a);
                break;
            }
            case b_not:
            {
                Object a=pop(object_stack);
                push(object_stack, to_int(is_falsy(a)));
                break;
            }
            case b_minus:
            {
                Object a=pop(object_stack);
                if(a.type==t_int){
                    push(object_stack, to_int(-a.int_value));
                } else if(a.type==t_float){
                    push(object_stack, to_float(-a.float_value));
                }
                break;
            }
            case b_minus_int:
            {
                Object a=pop(object_stack);
                push(object_stack, to_int(-a.int_value));
                break;
            }
            case b_minus_float:
            {
                Object a=pop(object_stack);
                push(object_stack, to_float(-a.float_value));
                break;
            }
            case b_jump_not:
            {
                Object condition=pop(object_stack);
                if(is_truthy(condition)){
                    dereference(E, &condition);
                    break;// go to the next line
                }
                dereference(E, &condition);
                // else go to case label underneath
            }
            case b_jump:
            {
                *pointer=E->bytecode_environment.executed_program->labels[instruction.uint_argument];
                break;
            }
            // these two instructions should always be called one after another
            case b_function_1:
            {
                Object f;
                function_init(E, &f);
                f.fp->enclosing_scope=E->scope;
                reference(&E->scope);
                f.fp->arguments_count=instruction.function_argument.arguments_count;
                f.fp->variadic=instruction.function_argument.is_variadic;
                f.fp->ftype=f_bytecode;
                // b_function_2 always follows b_function_1 so it can be read directly
                NEXT_INSTRUCTION
                HeapObject* program=(HeapObject*)E->bytecode_environment.executed_program->sub_programs[instruction.uint_argument];
                f.fp->source_pointer=program;
                heap_object_reference(program);
                push(object_stack, f);
                break;
            }
            case b_native_call_1:
            {
                unsigned arguments_count=instruction.uint_argument;
                // b_native_call_2 always follows b_native_call_1 so it can be read directly
                NEXT_INSTRUCTION
                ObjectSystemFunction f=(ObjectSystemFunction)instruction.uint_argument;
                Object* arguments=malloc(sizeof(Object)*arguments_count);
                for(int i=0; i<arguments_count; i++){
                    arguments[i]=pop(object_stack);
                }
                push(object_stack, f(E, null_const, arguments, arguments_count));
                free(arguments);
                break;
            }
            case b_tail_call:
            case b_call:
            {
                Object o=pop(object_stack);
                int provided_arguments=instruction.uint_argument;

                InstructionInformation call_information=program->information[E->bytecode_environment.pointer];
                TracebackPoint traceback_point={program->source_file_name, call_information.line};
                vector_push(&E->traceback, &traceback_point);

                #define RETURN(value) \
                    push(object_stack, value); \
                    vector_pop(&E->traceback); \
                    if(instruction.type==b_call) { \
                        break; \
                    } else { \
                        goto tail_call_return; \
                    }

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
                    RETURN(err) }
                
                if(o.type==t_null){
                    CALL_ERROR("Called function is null.");
                }
                
                // if object isn't a function it can be called through using monkey patching or table call field
                if(o.type!=t_function || o.fp->ftype==f_ast){
                    Object* arguments=malloc(sizeof(Object)*provided_arguments);
                    for (int i = provided_arguments-1; i >= 0 ; i--){
                        arguments[i]=pop(object_stack);
                    }
                    Object call_result=call(E, o, arguments, provided_arguments);
                    free(arguments);
                    RETURN(call_result)
                }
                int arguments_count_difference=provided_arguments-o.fp->arguments_count+o.fp->variadic;
                // check arguments count
                if(arguments_count_difference<0){
                    CALL_ERROR("Not enough arguments in function call, expected at least %i, given %i.", o.fp->arguments_count-o.fp->variadic, provided_arguments);
                } else if(!o.fp->variadic && arguments_count_difference>0) {
                    CALL_ERROR("Too many arguments in function call, expected %i, given %i.", o.fp->arguments_count, provided_arguments);
                }
                if(o.fp->ftype==f_native){
                    Object* arguments=malloc(sizeof(Object)*provided_arguments);
                    // items are on stack in reverse order, but native function expect them to be in normal order
                    for (int i = provided_arguments-1; i >= 0; i--){
                        arguments[i]=pop(object_stack);
                    }
                    Object call_result=o.fp->native_pointer(E, o.fp->enclosing_scope, arguments, provided_arguments);
                    free(arguments);
                    RETURN(call_result)
                } else if(o.fp->ftype==f_bytecode) {
                    if(o.fp->variadic){
                        // pack variadic arguments into a table and push this table back onto the stack
                        Object variadic_table;
                        table_init(E, &variadic_table);
                        for(int i=arguments_count_difference-1; i>=0; i--){
                            set(E, variadic_table, to_int(i), pop(object_stack));
                        }
                        push(object_stack, variadic_table);
                    }
                    if(instruction.type!=b_tail_call){
                        create_return_point(E, false);
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
                                NEW_ERROR(err, "COROUTINE_ERROR", o, "Yield from outside of coroutine.");
                                RETURN(err)
                            }
                            // simply return the value from stack top
                            if(E->options.debug){
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
                            if(E->options.debug){
                                E->bytecode_environment.debugger.running=false;
                                debugger(E);
                            }
                            RETURN(null_const)
                        }
                        case 2://  collect garbage
                        {
                            executor_collect_garbage(E);
                            RETURN(null_const)
                        }
                        default:
                            CALL_ERROR("Unknown special function of special_index %i.", o.fp->special_index)
                    }
                } else {
                    CALL_ERROR("Incorrect function type %i", o.fp->ftype);
                }
                #undef POP_ARGUMENTS
                #undef CALL_ERROR
                #undef RETURN
                dereference(E, &o);
                break;
            }
            tail_call_return:
            case b_end:
            case b_return:
            {
                if(E->options.debug){
                    debugger(E);
                }
                if(pop_return_point(E)){
                    if(E->coroutine!=NULL){
                        E->coroutine->state=co_finished;
                    }
                    return pop(object_stack);
                } else if(gc_should_run(E->object_system.gc) && !E->options.disable_garbage_collector){
                    executor_collect_garbage(E);
                }
                break;
            }
            case b_label:
                break;
            default:
                THROW_ERROR(WRONG_ARGUMENT_TYPE, "Uncatched bytecode instruction type: %i, number of instruction is: %i\n", instruction.type, *pointer);
        }
        (*pointer)++;
    }
}