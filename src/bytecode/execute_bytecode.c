#include "execute_bytecode.h"

#define STACK_SIZE 16

Object peek(vector* stack){
    return *(Object*)vector_top(stack);
}

void print_object_stack(Executor* E, const Stack* s){
    printf("Object stack:\n");
    int pointer=0;
    Object* casted_items=(Object*)s->items;
    while(pointer<stack_count(s)){
        USING_STRING(stringify(E, casted_items[pointer]), 
            printf("%s\n", str))
        pointer++;
    }
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
            argument=*(Object*)stack_index(&E->stack, stack_count(&E->stack)-1-i);
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
            current_call[i].constant=argument;
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
                argument=*(Object*)stack_index(&E->stack, stack_count(&E->stack)-1-i);
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
        inherit_scope(E, function_scope.tp, f->enclosing_scope);
    }
    reference(&function_scope);
    E->scope=function_scope;
    heap_object_reference((HeapObject*)f->source_pointer);

    if(f->ftype==f_bytecode){
        BytecodeProgram* program=(BytecodeProgram*)f->source_pointer;
        E->bytecode_environment.executed_program=program;
        E->bytecode_environment.pointer=0;
        executor_stack_forward_allocate(E, program->stack_depth);
    }
    
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

// returns true if execute_bytecode function should return value from stack instead of continuing
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
    vector_init(&environment->return_stack, sizeof(ReturnPoint), STACK_SIZE);
}

void bytecode_environment_deinit(BytecodeEnvironment* environment){
    vector_deinit(&environment->return_stack);
}

Object evaluate_string(Executor* E, const char* s, Object scope);

Object execute_bytecode(Executor* E){
    int* pointer=&E->bytecode_environment.pointer;// points to the current Instruction

    #define PROGRAM E->bytecode_environment.executed_program
    
    #define PUSH(object) \
        executor_stack_push_allocated(E, object)

    #define POP() \
        executor_stack_pop_allocated(E)

    #define NEXT_INSTRUCTION \
        (*pointer)++; \
        Instruction instruction=PROGRAM->code[*pointer];

    #define BYTECODE_ERROR(cause, message, ...) \
        {   Object err; \
            NEW_ERROR(err, "BYTECODE_ERROR", cause, message ", number of instruction is %i", ##__VA_ARGS__, *pointer); \
            PUSH(err); \
            goto return_label;   }

    while(true){
        if(E->options.debug){
            debugger_update(E, &E->debugger);
        }
        Instruction instruction=PROGRAM->code[*pointer];
        E->file=PROGRAM->source_file_name;
        E->line=PROGRAM->information[*pointer].line;
        E->column=PROGRAM->information[*pointer].column;
        
        switch(instruction.type){
            case b_no_op:
                break;
            case b_discard:
            {
                // remove item from the stack and delete it if it's not referenced
                Object top=POP();
                dereference(E, &top);
                break;
            }
            #define INDEX_STACK(index) (*(Object*)(E->stack.top-(index)))
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
                Object doubled=INDEX_STACK(0);
                reference(&doubled);
                PUSH(doubled);
                break;
            }
            #undef INDEX_STACK
            case b_load_string:
            {
                char* str=strdup(((char*)PROGRAM->constants)+instruction.uint_argument);
                PUSH(to_string(str));
                break;
            }
            case b_load_float:
            {
                PUSH(to_float(instruction.float_argument));
                break;
            }
            case b_load_int:
            {
                PUSH(to_int(instruction.int_argument));
                break;
            }
            case b_table_literal:
            {
                Object t;
                table_init(E, &t);
                PUSH(t);
                break;
            }
            case b_null:
            {
                PUSH(null_const);
                break;
            }
            case b_get:
            {
                Object key=POP();
                PUSH(get(E, E->scope, key));
                dereference(E, &key);
                break;
            }
            case b_table_get:
            {
                Object key=POP();
                Object indexed=POP();
                PUSH(get(E, indexed, key));
                dereference(E, &indexed);
                dereference(E, &key);
                break;
            }
            case b_set:
            {
                Object key=POP();
                Object value=POP();
                PUSH(set(E, E->scope, key, value));
                dereference(E, &key);
                dereference(E, &value);
                break;
            }
            case b_table_set:
            {
                Object key=POP();
                Object indexed=POP();
                Object value=POP();
                PUSH(set(E, indexed, key, value));
                dereference(E, &indexed);
                dereference(E, &key);
                dereference(E, &value);
                break;
            }
            case b_table_set_keep:
            {
                Object key=POP();
                Object value=POP();
                Object indexed=POP();
                Object set_result=set(E, indexed, key, value);
                PUSH(indexed);
                dereference(E, &set_result);
                dereference(E, &key);
                dereference(E, &value);
                break;
            }
            case b_enter_scope:
            {
                Object new_scope;
                table_init(E, &new_scope);
                inherit_current_scope(E, new_scope.tp);

                PUSH(E->scope);
                reference(&new_scope);
                E->scope=new_scope;
                break;
            }
            case b_leave_scope:
            {
                Object o=POP();
                if(o.type!=t_table){
                    BYTECODE_ERROR(o, "b_leave_scope: Object on stack isn't a table. It's type is: %i", o.type);
                } else {
                    dereference(E, &E->scope);
                    E->scope=o;
                }
                break;
            }
            case b_binary:
            {
                Object op=POP();
                Object a=POP();
                Object b=POP(); 
                if(op.type!=t_string){
                    dereference(E, &op);
                    dereference(E, &a);
                    dereference(E, &b);
                    BYTECODE_ERROR(op, "Operator must be of string type, it's type is %s", get_type_name(op.type));
                }
                PUSH(operator(E, a, b, op.text));
                dereference(E, &op);
                dereference(E, &a);
                dereference(E, &b);
                break;
            }
            #define OPERATOR_START \
                Object a=POP(); \
                Object b=POP();
            #define OPERATOR_RESULT(result) \
                PUSH(result);
            #define OPERATOR_END \
                dereference(E, &a); \
                dereference(E, &b); \
                break;
            #define OPERATOR_FALLBACK(op) \
                PUSH(operator(E, a, b, op));
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
                Object op=POP();
                Object a=POP();
                if(op.type!=t_string){
                    dereference(E, &op);
                    dereference(E, &a);
                    BYTECODE_ERROR(op, "Operator must be of string type, it's type is %s", get_type_name(op.type));
                }
                PUSH(operator(E, null_const, a, op.text));
                dereference(E, &op);
                dereference(E, &a);
                break;
            }
            case b_not:
            {
                Object a=POP();
                PUSH(to_int(is_falsy(a)));
                break;
            }
            case b_minus:
            {
                Object a=POP();
                if(a.type==t_int){
                    PUSH(to_int(-a.int_value));
                } else if(a.type==t_float){
                    PUSH(to_float(-a.float_value));
                } else {
                    PUSH(operator(E, null_const, a, "-"));
                }
                break;
            }
            case b_minus_int:
            {
                Object a=POP();
                PUSH(to_int(-a.int_value));
                break;
            }
            case b_minus_float:
            {
                Object a=POP();
                PUSH(to_float(-a.float_value));
                break;
            }
            case b_jump_not:
            {
                Object condition=POP();
                if(is_truthy(condition)){
                    dereference(E, &condition);
                    break;// go to the next line
                }
                dereference(E, &condition);
                // else go to case label underneath
            }
            case b_jump:
            {
                *pointer=PROGRAM->labels[instruction.uint_argument];
                break;
            }
            // b_function_2 always follows b_function_1 so they are handled in a single case label
            case b_function_1:
            {
                Object f;
                function_init(E, &f);
                f.fp->enclosing_scope=E->scope;
                reference(&E->scope);
                f.fp->arguments_count=instruction.function_argument.arguments_count;
                f.fp->optional_arguments_count=instruction.function_argument.optional_arguments_count;
                f.fp->variadic=instruction.function_argument.is_variadic;
                f.fp->ftype=f_bytecode;
                // b_function_2 always follows b_function_1 so it can be read directly
                NEXT_INSTRUCTION
                HeapObject* program=(HeapObject*)E->bytecode_environment.executed_program->sub_programs[instruction.uint_argument];
                f.fp->source_pointer=program;
                heap_object_reference(program);
                PUSH(f);
                break;
            }
            // same as above
            case b_native_call_1:
            {
                unsigned arguments_count=instruction.uint_argument;
                // b_native_call_2 always follows b_native_call_1 so it can be read directly
                NEXT_INSTRUCTION
                ObjectSystemFunction f=(ObjectSystemFunction)(long)instruction.uint_argument;
                Object* arguments=malloc(sizeof(Object)*arguments_count);
                for(int i=0; i<arguments_count; i++){
                    arguments[i]=POP();
                }
                PUSH(f(E, null_const, arguments, arguments_count));
                for(int i=0; i<arguments_count; i++){
                    dereference(E, &arguments[i]);
                }
                free(arguments);
                break;
            }
            case b_tail_call:
            case b_call:
            {
                Object o=POP();
                int provided_arguments=instruction.uint_argument;

                InstructionInformation call_information=PROGRAM->information[E->bytecode_environment.pointer];
                TracebackPoint traceback_point={PROGRAM->source_file_name, call_information.line};
                vector_push(&E->traceback, &traceback_point);

                #define RETURN(value) \
                    PUSH(value); \
                    vector_pop(&E->traceback); \
                    if(instruction.type==b_call) { \
                        break; \
                    } else { \
                        goto return_label; \
                    }

                #define POP_ARGUMENTS \
                    for (int i = 0; i < provided_arguments; i++){ \
                        Object argument=POP(); \
                            dereference(E, &argument); \
                    }
                #define CALL_ERROR(message, ...) \
                {   Object err; \
                    POP_ARGUMENTS \
                    NEW_ERROR(err, "CALL_ERROR", o, message, ##__VA_ARGS__); \
                    dereference(E, &o); \
                    RETURN(err) }
                
                if(o.type==t_null){
                    CALL_ERROR("Called function is null.");
                }
                
                // if object isn't a function it can be called through using monkey patching or table call field
                if(o.type!=t_function || o.fp->ftype==f_ast){
                    Object* arguments=malloc(sizeof(Object)*provided_arguments);
                    for (int i = provided_arguments-1; i >= 0 ; i--){
                        arguments[i]=POP();
                    }
                    Object call_result=call(E, o, arguments, provided_arguments);
                    for(int i=0; i<provided_arguments; i++){
                        dereference(E, &arguments[i]);
                    }
                    free(arguments);
                    RETURN(call_result)
                }
                int arguments_count_difference=provided_arguments-o.fp->arguments_count+o.fp->variadic;
                // check arguments count
                Object error;
                if(!is_arguments_count_correct(E, o.fp, provided_arguments, &error)){
                    return error;
                }
                if(o.fp->ftype==f_native){
                    Object* arguments=malloc(sizeof(Object)*provided_arguments);
                    // items are on stack in reverse order, but native function expect them to be in normal order
                    for (int i = provided_arguments-1; i >= 0; i--){
                        arguments[i]=POP();
                    }
                    Object call_result=o.fp->native_pointer(E, o.fp->enclosing_scope, arguments, provided_arguments);
                    for(int i=0; i<provided_arguments; i++){
                        dereference(E, &arguments[i]);
                    }
                    free(arguments);
                    RETURN(call_result)
                } else if(o.fp->ftype==f_bytecode) {
                    if(o.fp->optional_arguments_count>0){
                        int arguments_to_fill=o.fp->arguments_count-o.fp->variadic-provided_arguments;
                        for(int i=0; i<arguments_to_fill; i++){
                            reference(&E->undefined_argument);
                            PUSH(E->undefined_argument);
                        }
                    }
                    if(o.fp->variadic){
                        // pack variadic arguments into a table and push this table back onto the stack
                        Object variadic_table;
                        table_init(E, &variadic_table);
                        for(int i=arguments_count_difference-1; i>=0; i--){
                            set(E, variadic_table, to_int(i), POP());
                        }
                        PUSH(variadic_table);
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
                                dereference(E, &o);
                                RETURN(err)
                            }
                            // simply return the value from stack top
                            if(E->options.debug){
                                debugger_update(E, &E->debugger);
                            }
                            (*pointer)++;
                            if(provided_arguments==1){
                                return POP();
                            } else if(provided_arguments==0){
                                return null_const;
                            } else {
                                POP_ARGUMENTS
                                dereference(E, &o);
                                Object err;
                                NEW_ERROR(err, "COROUTINE_ERROR", o, "Incorrect number of arguments (%i) was passed to coroutine yield.", provided_arguments);
                                dereference(E, &o);
                                RETURN(err)
                            }
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
            return_label:
            case b_end:
            case b_return:
            {
                if(E->options.debug){
                    debugger_update(E, &E->debugger);
                }
                executor_stack_forward_deallocate(E, PROGRAM->stack_depth);
                if(pop_return_point(E)){
                    if(E->coroutine!=NULL){
                        E->coroutine->state=co_finished;
                    }
                    return POP();
                } else if(gc_should_run(E->object_system.gc) && !E->options.disable_garbage_collector){
                    executor_collect_garbage(E);
                }
                break;
            }
            case b_label:
                break;
            default:
                INCORRECT_ENUM_VALUE(InstructionType, instruction, instruction.type);
        }
        (*pointer)++;
    }
}