#include "bytecode_to_lightning.h"
#include "../execution/execution.h"

#ifdef HAS_LIGHTNING

static jit_state_t *_jit;

#define OBJECT_STACK_REGISTER JIT_V0

void operator_lightning_wrapper(Executor* E, Object* a, Object* b, const char* op, Object* result){
    *result=operator(E, *a, *b, op);
}

void call_lightning_wrapper(Executor* E, Object* o, Object* arguments, int arguments_count, Object* result){
    *result=call(E, *o, arguments, arguments_count);
}

void get_lightning_wrapper(Executor* E, Object* o, Object* key, Object* result){
    *result=get(E, *o, *key);
}

void set_lightning_wrapper(Executor* E, Object* o, Object* key, Object* value, Object* result){
    *result=set(E, *o, *key, *value);
}

void inherit_scope_lightning_wrapper(Executor* E, Object* scope, Object* base){
    inherit_scope(E, scope->tp, *base);
}

bool is_falsy_lightning_wrapper(Object* o){
    return is_falsy(*o);
}

void stack_remove_items(Executor* E, vector* stack, int count){
    for(int i=0; i<count; i++){
        Object o=objects_vector_pop(stack);
        dereference(E, &o);
    }
}

// used for inspecting objects in debugger
int catch_in_gdb(Object* o){
   return o->int_value;
}

#define PRINT(message) \
    jit_prepare(); \
    jit_pushargi((long)message); \
    jit_finishi(printf);

void instruction_to_lightning(Executor* E, 
                          Instruction* instruction, 
                          int temporary_offset,
                          int executor_offset,
                          int program_offset, 
                          jit_node_t** jit_labels) {
    
    #define GET_TEMPORARY(register) \
        jit_addi(register, JIT_FP, temporary_offset);
    
    #define GET_EXECUTOR(register) \
        jit_ldxi_l(register, JIT_FP, executor_offset);

    #define GET_PROGRAM(register) \
        jit_ldxi_l(register, JIT_FP, program_offset);

    // TODO: reference/dereference
    #define POP(register) \
        jit_prepare(); \
        jit_pushargr(OBJECT_STACK_REGISTER); \
        jit_finishi(vector_pop); \
        jit_retval(register);

    #define REFERENCE(register) \
        jit_prepare(); \
        jit_pushargr(register); \
        jit_finishi(reference);

    #define DEREFERENCE(register, executor_temporary) \
        GET_EXECUTOR(executor_temporary); \
        jit_prepare(); \
        jit_pushargr(executor_temporary); \
        jit_pushargr(register); \
        jit_finishi(dereference);

    #define PUSH(register) \
        jit_prepare(); \
        jit_pushargr(OBJECT_STACK_REGISTER); \
        jit_pushargr(register); \
        jit_finishi(vector_push); \
        REFERENCE(register)

    #define INDEX_STACK(register, index) \
        jit_prepare(); \
        jit_pushargr(OBJECT_STACK_REGISTER); \
        jit_finishi(vector_top); \
        jit_retval_l(register); \
        if(index>0){ \
            jit_subi(register, register, (index)*sizeof(Object)); \
        } else { \
            jit_retval(register); \
        }

    #define CATCH(register) \
        jit_prepare(); \
        jit_pushargr(register); \
        jit_finishi(catch_in_gdb);

    // E->bytecode_environment.object_stack
    #define PRINT_STACK(executor, stack_temporary) \
        jit_addi(stack_temporary, executor, FIELD_OFFSET(Executor, bytecode_environment)+FIELD_OFFSET(BytecodeEnvironment, object_stack)); \
        jit_prepare(); \
        jit_pushargr(executor); \
        jit_pushargr(stack_temporary); \
        jit_finishi(print_object_stack);

    // pushes null to the stack and writes  pointer to it to the register
    // vector_push(OBJECT_STACK_REGISTER, null_const);
    // register=vector_top(OBJECT_STACK_REGISTER);
    #define PUSH_NULL(register) \
        /*vector_push(OBJECT_STACK_REGISTER, null_const);*/ \
        jit_prepare(); \
        jit_pushargr(OBJECT_STACK_REGISTER); \
        jit_pushargi((long)&null_const); \
        jit_finishi(vector_push); \
        /* JIT_V1=vector_top(&object_stack);*/ \
        jit_prepare(); \
        jit_pushargr(OBJECT_STACK_REGISTER); \
        jit_finishi(vector_top); \
        jit_retval(register);

    #define MOVE_OBJECT(destination, source) \
        jit_prepare(); \
        jit_pushargr(destination); \
        jit_pushargr(source); \
        jit_pushargi(sizeof(Object)); \
        jit_finishi(memcpy);

    #define GET_SCOPE(register, executor) \
        jit_addi(register, executor, FIELD_OFFSET(Executor, scope));

    bool debug=true;
    #define CASE(instruction) \
        case instruction: \
            if(debug) { \
                PRINT(#instruction "\n"); \
            }

    switch (instruction->type){
        CASE(b_no_op)
            break;
        CASE(b_push_to_top)
            // JIT_V1 is loop iterator variable i
            jit_movi(JIT_V1, instruction->uint_argument);
            GET_TEMPORARY(JIT_V2)
            // *temporary=*INDEX_STACK(i)
            INDEX_STACK(JIT_V3, JIT_V1)
            MOVE_OBJECT(JIT_V2, JIT_V3)
            // *INDEX_STACK(i)=*INDEX_STACK(i-1)
            // JIT_R2=i-1
            jit_subi(JIT_R2, JIT_V1, 1);
            INDEX_STACK(JIT_R1, JIT_R2)
            MOVE_OBJECT(JIT_V3, JIT_R1)
            // *INDEX_STACK(i-1)=*temporary
            MOVE_OBJECT(JIT_R1, JIT_V2)
            break;
        CASE(b_swap)
            // *temporary=*INDEX_STACK(instruction->swap_argument.left)
            GET_TEMPORARY(JIT_V1)
            INDEX_STACK(JIT_V2, instruction->swap_argument.left)
            MOVE_OBJECT(JIT_V1, JIT_V2)
            // *INDEX_STACK(instruction->swap_argument.left)=*INDEX_STACK(instruction->swap_argument.right)
            INDEX_STACK(JIT_V3, instruction->swap_argument.right)
            MOVE_OBJECT(JIT_V2, JIT_V3)
            // *INDEX_STACK(instruction->swap_argument.right)=*temporary
            MOVE_OBJECT(JIT_V3, JIT_V1)
            break;
        CASE(b_discard)
            // JIT_V1=vector_pop(OBJECT_STACK_REGISTER);
            POP(JIT_V1)
            // JIT_V2=E;
            DEREFERENCE(JIT_V1, JIT_V2)
            break;
        CASE(b_double)
            GET_TEMPORARY(JIT_V1)
            // get stack top to JIT_V2
            jit_prepare();
            jit_pushargr(OBJECT_STACK_REGISTER);
            jit_finishi(vector_top);
            jit_retval(JIT_V2);
            MOVE_OBJECT(JIT_V1, JIT_V2)
            // push copied object back to stack
            PUSH(JIT_V1)
            break;
        CASE(b_null)
            // vector_push(OBJECT_STACK_REGISTER, null_const);
            jit_prepare();
            jit_pushargr(OBJECT_STACK_REGISTER);
            jit_pushargi((long)&null_const);
            jit_finishi(vector_push);
            break;
        
        #define PUSH_TYPED(result_register, object_type, temporary1, temporary2) \
            PUSH_NULL(result_register) \
            /* vector_top(&object_stack)->type=t_int; */ \
            jit_addi(temporary1, result_register, FIELD_OFFSET(Object, type)); \
            jit_movi(temporary2, object_type); \
            jit_str_l(temporary1, temporary2);
        CASE(b_load_int)
            PUSH_TYPED(JIT_V1, t_int, JIT_V2, JIT_V3)
            // vector_top(&object_stack)->int_value=instruction.int_argument
            jit_addi(JIT_V2, JIT_V1, FIELD_OFFSET(Object, int_value));
            jit_movi(JIT_V3, instruction->int_argument);
            jit_str(JIT_V2, JIT_V3);
            break;
        CASE(b_load_float)
            PUSH_TYPED(JIT_V1, t_float, JIT_V2, JIT_V3)
            // vector_top(&object_stack)->float_value=instruction.float_argument;
            jit_addi(JIT_V2, JIT_V1, FIELD_OFFSET(Object, float_value));
            jit_movi_f(JIT_V0, instruction->float_argument);
            jit_str_f(JIT_V2, JIT_V0);
            break;
        CASE(b_load_string)
            PUSH_TYPED(JIT_V1, t_string, JIT_V2, JIT_V3)
            // vector_top(&object_stack)->text=instruction.float_argument;
            jit_addi(JIT_V2, JIT_V1, FIELD_OFFSET(Object, text));
            // get bytecode_program argument to JIT_V3
            GET_PROGRAM(JIT_V3)
            // get constants to JIT_R2
            jit_ldxi_l(JIT_R2, JIT_V3, FIELD_OFFSET(BytecodeProgram, constants));
            // get pointer to string constant to JIT_R1
            jit_addi(JIT_R1, JIT_R2, instruction->uint_argument);
            // write it to Object's text field
            jit_str_l(JIT_V2, JIT_R1);
            REFERENCE(JIT_V1)
            break;
        CASE(b_table_literal)
            // push null on stack and call table_init on it
            GET_EXECUTOR(JIT_V1)
            PUSH_NULL(JIT_V2)
            jit_prepare();
            jit_pushargr(JIT_V1);
            jit_pushargr(JIT_V2);
            jit_finishi(table_init);
            break;
        #undef PUSH_TYPED

        CASE(b_enter_scope)
            GET_EXECUTOR(JIT_V1)
            GET_TEMPORARY(JIT_V2)
            // call table_init
            jit_prepare();
            jit_pushargr(JIT_V1);
            jit_pushargr(JIT_V2);
            jit_finishi(table_init);
            // call inherit_scope
            GET_SCOPE(JIT_V3, JIT_V1)
            jit_prepare();
            jit_pushargr(JIT_V1);
            jit_pushargr(JIT_V2);
            jit_pushargr(JIT_V3);
            jit_finishi(inherit_scope_lightning_wrapper);
            // push older scope on the stack
            PUSH(JIT_V3)
            // change the scope
            REFERENCE(JIT_V2)
            DEREFERENCE(JIT_V3, JIT_R2)
            MOVE_OBJECT(JIT_V3, JIT_V2)
            break;
        CASE(b_leave_scope)
            GET_EXECUTOR(JIT_V1)
            GET_SCOPE(JIT_V2, JIT_V1)
            DEREFERENCE(JIT_V2, JIT_R2)
            POP(JIT_V3)
            REFERENCE(JIT_V3)
            // change the scope to popped object
            MOVE_OBJECT(JIT_V2, JIT_V3)
            break;
        CASE(b_get)
            GET_EXECUTOR(JIT_V1)
            GET_SCOPE(JIT_V2, JIT_V1)
            POP(JIT_V3)// key
            GET_TEMPORARY(JIT_R2)
            jit_prepare();
            jit_pushargr(JIT_V1);
            jit_pushargr(JIT_V2);
            jit_pushargr(JIT_V3);
            jit_pushargr(JIT_R2);
            jit_finishi(get_lightning_wrapper);
            PUSH(JIT_R2)
            break;
        CASE(b_set)
            GET_EXECUTOR(JIT_V1)
            GET_SCOPE(JIT_V2, JIT_V1)
            POP(JIT_V3)// key
            POP(JIT_R2)// value
            GET_TEMPORARY(JIT_R1)
            jit_prepare();
            jit_pushargr(JIT_V1);
            jit_pushargr(JIT_V2);
            jit_pushargr(JIT_V3);
            jit_pushargr(JIT_R2);
            jit_pushargr(JIT_R1);
            jit_finishi(set_lightning_wrapper);
            PUSH(JIT_R1)
            break;
        CASE(b_table_set_keep)
            GET_EXECUTOR(JIT_V1)
            POP(JIT_V2)// key
            POP(JIT_V3)// value
            POP(JIT_R2)// table
            GET_TEMPORARY(JIT_R1)
            jit_prepare();
            jit_pushargr(JIT_V1);
            jit_pushargr(JIT_R2);
            jit_pushargr(JIT_V2);
            jit_pushargr(JIT_V3);
            jit_pushargr(JIT_R1);
            jit_finishi(set_lightning_wrapper);
            jit_prepare();
            jit_pushargr(JIT_V1);
            jit_pushargr(JIT_R1);
            jit_finishi(dereference);
            PUSH(JIT_R2)// push table back on stack
            break;
        CASE(b_table_set)
            GET_EXECUTOR(JIT_V1)
            POP(JIT_V2)// key
            POP(JIT_V3)// value
            POP(JIT_R2)// table
            GET_TEMPORARY(JIT_R1)
            jit_prepare();
            jit_pushargr(JIT_V1);
            jit_pushargr(JIT_R2);
            jit_pushargr(JIT_V2);
            jit_pushargr(JIT_V3);
            jit_pushargr(JIT_R1);
            jit_finishi(set_lightning_wrapper);
            PUSH(JIT_R1)// push set return value on stack
            break;
        CASE(b_table_get)
            GET_EXECUTOR(JIT_V1)
            POP(JIT_V2)// key
            POP(JIT_V3)// table
            GET_TEMPORARY(JIT_R2)
            jit_prepare();
            jit_pushargr(JIT_V1);
            jit_pushargr(JIT_V3);
            jit_pushargr(JIT_V2);
            jit_pushargr(JIT_R2);
            jit_finishi(get_lightning_wrapper);
            PUSH(JIT_R2)// push get return value on stack
            break;
        CASE(b_function_1)
            GET_EXECUTOR(JIT_V1)
            PUSH_NULL(JIT_V2)
            jit_prepare();
            jit_pushargr(JIT_V1);
            jit_pushargr(JIT_V2);
            jit_finishi(function_init);
            // JIT_V3 is a pointer to Function struct
            jit_ldxi_l(JIT_V3, JIT_V2, FIELD_OFFSET(Object, fp));
            // JIT_R2 is a pointer to enclosing scope
            jit_addi(JIT_R2, JIT_V3, FIELD_OFFSET(Function, enclosing_scope));
            GET_SCOPE(JIT_R1, JIT_V1)
            MOVE_OBJECT(JIT_R2, JIT_R1)
            REFERENCE(JIT_R2)
            // JIT_R2 is a pointer to arguments_count
            jit_addi(JIT_R2, JIT_V3, FIELD_OFFSET(Function, arguments_count));
            jit_movi(JIT_R1, instruction->function_argument.arguments_count);
            jit_str_i(JIT_R2, JIT_R1);
            // JIT_R2 is a pointer to variadic
            jit_addi(JIT_R2, JIT_V3, FIELD_OFFSET(Function, variadic));
            jit_movi(JIT_R1, instruction->function_argument.is_variadic);
            jit_str_i(JIT_R2, JIT_R1);
            // JIT_R2 is a pointer to ftype
            jit_addi(JIT_R2, JIT_V3, FIELD_OFFSET(Function, ftype));
            jit_movi(JIT_R1, f_bytecode);
            jit_str_i(JIT_R2, JIT_R1);
            break;
        CASE(b_function_2)
            // get sub_program
            // JIT_V1 is pointer to current bytecode program
            GET_PROGRAM(JIT_V1)
            // JIT_V2 points to the first sub_program, JIT_V2=program->sub_programs[0]
            jit_ldxi_l(JIT_V2, JIT_V1, FIELD_OFFSET(BytecodeProgram, sub_programs));
            // JIT_V2 is moved by uint_argument
            jit_ldxi_l(JIT_V2, JIT_V2, instruction->uint_argument*sizeof(BytecodeProgram*));

            INDEX_STACK(JIT_V3, 0)
            // JIT_R2 is a pointer to Function struct
            jit_ldxi_l(JIT_R2, JIT_V3, FIELD_OFFSET(Object, fp));
            // JIT_R2 is a pointer to source_pointer field
            jit_addi(JIT_R2, JIT_R2, FIELD_OFFSET(Function, source_pointer));
            jit_str_l(JIT_R2, JIT_V2);
            // reference the program
            
            jit_prepare();
            jit_pushargr(JIT_V2);
            jit_finishi(heap_object_reference);
            break;
        CASE(b_binary)
            GET_EXECUTOR(JIT_V1)
            // JIT_V2 is operator name
            POP(JIT_V2)
            POP(JIT_V3)
            POP(JIT_R2)
            // JIT_R1 will contain text field of operator object
            jit_ldxi_l(JIT_R1, JIT_V2, FIELD_OFFSET(Object, text));
            GET_TEMPORARY(JIT_R2)
            jit_prepare();
            jit_pushargr(JIT_V1);
            jit_pushargr(JIT_V3);
            jit_pushargr(JIT_R2);
            jit_pushargr(JIT_R1);
            jit_pushargr(JIT_R2);
            jit_finishi(operator_lightning_wrapper);
            PUSH(JIT_R2)
            break;
        CASE(b_prefix)
            GET_EXECUTOR(JIT_V1)
            POP(JIT_V2)
            POP(JIT_V3)
            GET_TEMPORARY(JIT_R2)
            jit_prepare();
            jit_pushargr(JIT_V1);
            jit_pushargi((long)&null_const);
            jit_pushargr(JIT_V3);
            jit_pushargi(JIT_V2);
            jit_pushargr(JIT_R2);
            jit_finishi(operator_lightning_wrapper);
            PUSH(JIT_R2)
            break;
        // TODO: implement each of these in lightning
        #define BINARY(instruction, operator) \
            CASE(instruction) \
                GET_EXECUTOR(JIT_V1) \
                POP(JIT_V2) \
                POP(JIT_V3) \
                GET_TEMPORARY(JIT_R2) \
                jit_prepare(); \
                jit_pushargr(JIT_V1); \
                jit_pushargr(JIT_V2); \
                jit_pushargr(JIT_V3); \
                jit_pushargi((long)operator); \
                jit_pushargr(JIT_R2); \
                jit_finishi(operator_lightning_wrapper); \
                PUSH(JIT_R2) \
                break;
        #define PREFIX(instruction, operator) \
            CASE(instruction) \
                GET_EXECUTOR(JIT_V1) \
                POP(JIT_V2) \
                GET_TEMPORARY(JIT_V3) \
                jit_prepare(); \
                jit_pushargr(JIT_V1); \
                jit_pushargi((long)&null_const); \
                jit_pushargr(JIT_V2); \
                jit_pushargi((long)operator); \
                jit_pushargr(JIT_V3); \
                jit_finishi(operator_lightning_wrapper); \
                PUSH(JIT_V3) \
                break;
        OPERATOR_INSTRUCTIONS
        #undef BINARY
        #undef PREFIX
        CASE(b_jump)
            // TODO: Jump back (although it isn't used in current version of bytecode generator)
            jit_labels[instruction->uint_argument]=jit_forward();
            jit_patch_at(jit_labels[instruction->uint_argument], jit_jmpi());
            break;
        CASE(b_jump_not)
            POP(JIT_V1)
            jit_prepare();
            jit_pushargr(JIT_V1);
            jit_finishi(is_falsy_lightning_wrapper);
            jit_retval(JIT_V2);
            jit_labels[instruction->uint_argument]=jit_forward();
            jit_patch_at(jit_labels[instruction->uint_argument], jit_beqi(JIT_V2, true));
            break;
        CASE(b_label)
            jit_patch(jit_labels[instruction->uint_argument]);
            break;
        CASE(b_call)
        CASE(b_tail_call)
            GET_EXECUTOR(JIT_V1)
            // pop callable
            POP(JIT_V2)
            // get pointer to arguments
            INDEX_STACK(JIT_V3, (int)instruction->uint_argument-1)
            GET_TEMPORARY(JIT_R2)
            jit_prepare();
            jit_pushargr(JIT_V1);
            jit_pushargr(JIT_V2);
            jit_pushargr(JIT_V3);
            jit_pushargi(instruction->uint_argument);
            jit_pushargr(JIT_R2);
            jit_finishi(call_lightning_wrapper);
            DEREFERENCE(JIT_V2, JIT_V1)
            jit_prepare();
            jit_pushargr(JIT_V1);
            jit_pushargr(OBJECT_STACK_REGISTER);
            jit_pushargi(instruction->uint_argument);
            jit_finishi(stack_remove_items);
            PUSH(JIT_R2)
            if(instruction->type==b_call) {
                break;
            }
            // intentional fallthrough if instruction is of type b_tail_call
        CASE(b_end)
        CASE(b_return)
            jit_ret();
            break;
        default:
            THROW_ERROR(BYTECODE_ERROR, "Don't know how to compile instruction %s.", INSTRUCTION_NAMES[instruction->type]);
            break;
    }
    #undef CASE
}

void compile_bytecode_program(Executor* E, BytecodeProgram* program){
    
    //creates a new instance of the compiler and ask it to assign result to program->compiled
    
    init_jit(program->source_file_name);
    _jit = jit_new_state();

    jit_note(__FILE__, __LINE__);
    jit_prolog();
    
    // allocate space for one object on the stack frame, it is used by some operations
    int temporary_offset=jit_allocai(sizeof(Object));
    int executor_offset=jit_allocai(sizeof(Executor*));
    int program_offset=jit_allocai(sizeof(BytecodeProgram*));

    #define WRITE_ARGUMENT(argument_register, allocated_register, offset) \
        jit_getarg(argument_register, jit_arg()); \
        jit_addi(allocated_register, JIT_FP, offset); \
        jit_str_l(allocated_register, argument_register); \

    WRITE_ARGUMENT(JIT_V1, JIT_V2, executor_offset)
    jit_ldr_l(OBJECT_STACK_REGISTER, JIT_V2);// get object_stack to OBJECT_STACK_REGISTER
    jit_addi(OBJECT_STACK_REGISTER, OBJECT_STACK_REGISTER, FIELD_OFFSET(Executor, stack));
    WRITE_ARGUMENT(JIT_V1, JIT_V2, program_offset)

    PRINT("-- lightning function begin\n")

    jit_node_t** jit_labels=malloc(sizeof(jit_node_t*)*program->labels_count);
    for(int i=0; true; i++) {
        instruction_to_lightning(E, &program->code[i], temporary_offset,
                             executor_offset, program_offset, jit_labels);
        if(program->code[i].type==b_end){
            break;
        }
    }
    /*for(int i=0; i<program->labels_count; i++){
        jit_link(jit_labels[i]);
    }*/
    free(jit_labels);

    jit_epilog();
    jit_note(__FILE__, __LINE__);
    program->compiled=jit_emit();

    //jit_destroy_state();
    finish_jit();
}
#else
void compile_bytecode_program(Executor* E, BytecodeProgram* program){}

#endif