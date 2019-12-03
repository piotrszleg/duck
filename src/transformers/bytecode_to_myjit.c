#include "bytecode_to_myjit.h"
#include "../execution/execution.h"

#define OBJECT_STACK_REGISTER R(0)

void operator_myjit_wrapper(Executor* E, Object* a, Object* b, const char* op, Object* result){
    *result=operator(E, *a, *b, op);
}

void call_myjit_wrapper(Executor* E, Object* o, Object* arguments, int arguments_count, Object* result){
    *result=call(E, *o, arguments, arguments_count);
}

void get_myjit_wrapper(Executor* E, Object* o, Object* key, Object* result){
    *result=get(E, *o, *key);
}

void set_myjit_wrapper(Executor* E, Object* o, Object* key, Object* value, Object* result){
    *result=set(E, *o, *key, *value);
}

void inherit_scope_myjit_wrapper(Executor* E, Object* scope, Object* base){
    inherit_scope(E, *scope, *base);
}

bool is_falsy_myjit_wrapper(Object* o){
    return is_falsy(*o);
}

int catch_in_gdb(Object* o){
   return o->int_value;
}

void instruction_to_myjit(Executor* E, struct jit * C, Instruction* instruction, int temporary_offset, jit_op** jit_labels) {
    // TODO: reference/dereference
    #define POP(register) \
        jit_prepare(C); \
        jit_putargr(C, OBJECT_STACK_REGISTER); \
        jit_call(C, vector_pop); \
        jit_retval(C, register);

    #define REFERENCE(register) \
        jit_prepare(C); \
        jit_putargr(C, register); \
        jit_call(C, reference);

    #define DEREFERENCE(register, executor_temporary) \
        jit_getarg(C, executor_temporary, 0); \
        jit_prepare(C); \
        jit_putargr(C, executor_temporary); \
        jit_putargr(C, register); \
        jit_call(C, dereference);

    #define PUSH(register) \
        jit_prepare(C); \
        jit_putargr(C, OBJECT_STACK_REGISTER); \
        jit_putargr(C, register); \
        jit_call(C, vector_push); \
        REFERENCE(register)

    #define GET_TEMPORARY(register) \
        jit_addi(C, register, R_FP, temporary_offset);

    #define INDEX_STACK(register, index) \
        jit_prepare(C); \
        jit_putargr(C, OBJECT_STACK_REGISTER); \
        jit_call(C, vector_top); \
        if(index>0){ \
            jit_subi(C, register, R_OUT, index*sizeof(Object)); \
        } else { \
            jit_retval(C, register); \
        }

    #define CATCH(register) \
        jit_prepare(C); \
        jit_putargr(C, register); \
        jit_call(C, catch_in_gdb);

    // E->bytecode_environment.object_stack
    #define PRINT_STACK(executor, stack_temporary) \
        jit_addi(C, stack_temporary, executor, FIELD_OFFSET(Executor, bytecode_environment)+FIELD_OFFSET(BytecodeEnvironment, object_stack)); \
        jit_prepare(C); \
        jit_putargr(C, executor); \
        jit_putargr(C, stack_temporary); \
        jit_call(C, print_object_stack);

    // pushes null to the stack and writes  pointer to it to the register
    // vector_push(OBJECT_STACK_REGISTER, null_const);
    // register=vector_top(OBJECT_STACK_REGISTER);
    #define PUSH_NULL(register) \
        /*vector_push(OBJECT_STACK_REGISTER, null_const);*/ \
        jit_prepare(C); \
        jit_putargr(C, OBJECT_STACK_REGISTER); \
        jit_putargi(C, &null_const); \
        jit_call(C, vector_push); \
        /* R(1)=vector_top(&object_stack);*/ \
        jit_prepare(C); \
        jit_putargr(C, OBJECT_STACK_REGISTER); \
        jit_call(C, vector_top); \
        jit_retval(C, register);

    #define MOVE_OBJECT(destination, source) \
        jit_prepare(C); \
        jit_putargr(C, destination); \
        jit_putargr(C, source); \
        jit_putargi(C, sizeof(Object)); \
        jit_call(C, memcpy);

    #define GET_EXECUTOR(register) \
        jit_getarg(C, register, 0);

    #define GET_SCOPE(register, executor) \
        jit_addi(C, register, executor, FIELD_OFFSET(Executor, scope));

    bool debug=true;
    
    #define CASE(instruction) \
        case instruction: \
            if(debug) { \
                jit_msg(C, #instruction "\n"); \
            }

    switch (instruction->type){
        CASE(b_no_op)
            break;
        CASE(b_push_to_top)
            // R(1) is loop iterator variable i
            jit_movi(C, R(1), instruction->uint_argument);
            GET_TEMPORARY(R(2))
            // *temporary=*INDEX_STACK(i)
            INDEX_STACK(R(3), R(1))
            MOVE_OBJECT(R(2), R(3))
            // *INDEX_STACK(i)=*INDEX_STACK(i-1)
            // R(4)=i-1
            jit_subi(C, R(4), R(1), 1);
            INDEX_STACK(R(5), R(4))
            MOVE_OBJECT(R(3), R(5))
            // *INDEX_STACK(i-1)=*temporary
            MOVE_OBJECT(R(5), R(2))
            break;
        CASE(b_swap)
            // *temporary=*INDEX_STACK(instruction->swap_argument.left)
            GET_TEMPORARY(R(1))
            INDEX_STACK(R(2), instruction->swap_argument.left)
            MOVE_OBJECT(R(1), R(2))
            // *INDEX_STACK(instruction->swap_argument.left)=*INDEX_STACK(instruction->swap_argument.right)
            INDEX_STACK(R(3), instruction->swap_argument.right)
            MOVE_OBJECT(R(2), R(3))
            // *INDEX_STACK(instruction->swap_argument.right)=*temporary
            MOVE_OBJECT(R(3), R(1))
            break;
        CASE(b_discard)
            // R(1)=vector_pop(OBJECT_STACK_REGISTER);
            POP(R(1))
            // R(2)=E;
            jit_getarg(C, R(2), 0);
            // dereference(R(2), R(1));
            jit_prepare(C);
            jit_putargr(C, R(2));
            jit_putargr(C, R(1));
            jit_call(C, dereference);
            break;
        CASE(b_double)
            GET_TEMPORARY(R(1))
            // get stack top to R(2)
            jit_prepare(C);
            jit_putargr(C, OBJECT_STACK_REGISTER);
            jit_call(C, vector_top);
            jit_retval(C, R(2));
            // memcpy(R(1), R(2), sizeof(Object))
            jit_prepare(C);
            jit_putargr(C, R(1));
            jit_putargr(C, R(2));
            jit_putargi(C, sizeof(Object));
            jit_call(C, memcpy);
            // push copied object back to stack
            PUSH(R(1))
            break;
        CASE(b_null)
            // vector_push(OBJECT_STACK_REGISTER, null_const);
            jit_prepare(C);
            jit_putargr(C, OBJECT_STACK_REGISTER);
            jit_putargi(C, &null_const);
            jit_call(C, vector_push);
            break;
        
        #define PUSH_TYPED(result_register, object_type, temporary1, temporary2) \
            PUSH_NULL(result_register) \
            /* vector_top(&object_stack)->type=t_int; */ \
            jit_addi(C, temporary1, result_register, FIELD_OFFSET(Object, type)); \
            jit_movi(C, temporary2, object_type); \
            jit_str(C, temporary1, temporary2, sizeof(ObjectType));
        CASE(b_load_int)
            PUSH_TYPED(R(1), t_int, R(2), R(3))
            // vector_top(&object_stack)->int_value=instruction.int_argument
            jit_addi(C, R(2), R(1), FIELD_OFFSET(Object, int_value));
            jit_movi(C, R(3), instruction->int_argument);
            jit_str(C, R(2), R(3), sizeof(int));
            break;
        CASE(b_load_float)
            PUSH_TYPED(R(1), t_float, R(2), R(3))
            // vector_top(&object_stack)->float_value=instruction.float_argument;
            jit_addi(C, R(2), R(1), FIELD_OFFSET(Object, float_value));
            jit_fmovi(C, FR(0), instruction->float_argument);
            jit_fstr(C, R(2), FR(0), sizeof(float));
            break;
        CASE(b_load_string)
            PUSH_TYPED(R(1), t_string, R(2), R(3))
            // vector_top(&object_stack)->text=instruction.float_argument;
            jit_addi(C, R(2), R(1), FIELD_OFFSET(Object, text));
            // get bytecode_program argument to R(3)
            jit_getarg(C, R(3), 1);
            // get constants to R(4)
            jit_ldxi(C, R(4), R(3), FIELD_OFFSET(BytecodeProgram, constants), sizeof(char*));
            // get pointer to string constant to R(5)
            jit_addi(C, R(5), R(4), instruction->uint_argument);
            // write it to Object's text field
            jit_str(C, R(2), R(5), sizeof(char*));
            REFERENCE(R(1))
            break;
        CASE(b_table_literal)
            // push null on stack and call table_init on it
            GET_EXECUTOR(R(1))
            PUSH_NULL(R(2))
            jit_prepare(C);
            jit_putargr(C, R(1));
            jit_putargr(C, R(2));
            jit_call(C, table_init);
            break;
        #undef PUSH_TYPED

        CASE(b_enter_scope)
            GET_EXECUTOR(R(1))
            GET_TEMPORARY(R(2))
            // call table_init
            jit_prepare(C);
            jit_putargr(C, R(1));
            jit_putargr(C, R(2));
            jit_call(C, table_init);
            // call inherit_scope
            GET_SCOPE(R(3), R(1))
            jit_prepare(C);
            jit_putargr(C, R(1));
            jit_putargr(C, R(2));
            jit_putargr(C, R(3));
            jit_call(C, inherit_scope_myjit_wrapper);
            // push older scope on the stack
            PUSH(R(3))
            // change the scope
            REFERENCE(R(2))
            DEREFERENCE(R(3), R(4))
            MOVE_OBJECT(R(3), R(2))
            break;
        CASE(b_leave_scope)
            GET_EXECUTOR(R(1))
            GET_SCOPE(R(2), R(1))
            DEREFERENCE(R(2), R(4))
            POP(R(3))
            REFERENCE(R(3))
            // change the scope to popped object
            MOVE_OBJECT(R(2), R(3))
            break;
        CASE(b_get)
            GET_EXECUTOR(R(1))
            GET_SCOPE(R(2), R(1))
            POP(R(3))// key
            GET_TEMPORARY(R(4))
            jit_prepare(C);
            jit_putargr(C, R(1));
            jit_putargr(C, R(2));
            jit_putargr(C, R(3));
            jit_putargr(C, R(4));
            jit_call(C, get_myjit_wrapper);
            PUSH(R(4))
            break;
        CASE(b_set)
            GET_EXECUTOR(R(1))
            GET_SCOPE(R(2), R(1))
            POP(R(3))// key
            POP(R(4))// value
            GET_TEMPORARY(R(5))
            jit_prepare(C);
            jit_putargr(C, R(1));
            jit_putargr(C, R(2));
            jit_putargr(C, R(3));
            jit_putargr(C, R(4));
            jit_putargr(C, R(5));
            jit_call(C, set_myjit_wrapper);
            PUSH(R(5))
            break;
        CASE(b_table_set_keep)
            GET_EXECUTOR(R(1))
            POP(R(2))// key
            POP(R(3))// value
            POP(R(4))// table
            GET_TEMPORARY(R(5))
            jit_prepare(C);
            jit_putargr(C, R(1));
            jit_putargr(C, R(4));
            jit_putargr(C, R(2));
            jit_putargr(C, R(3));
            jit_putargr(C, R(5));
            jit_call(C, set_myjit_wrapper);
            jit_prepare(C);
            jit_putargr(C, R(1));
            jit_putargr(C, R(5));
            jit_call(C, destroy_unreferenced);
            PUSH(R(4))// push table back on stack
            break;
        CASE(b_table_set)
            GET_EXECUTOR(R(1))
            POP(R(2))// key
            POP(R(3))// value
            POP(R(4))// table
            GET_TEMPORARY(R(5))
            jit_prepare(C);
            jit_putargr(C, R(1));
            jit_putargr(C, R(4));
            jit_putargr(C, R(2));
            jit_putargr(C, R(3));
            jit_putargr(C, R(5));
            jit_call(C, set_myjit_wrapper);
            PUSH(R(5))// push set return value on stack
            break;
        CASE(b_table_get)
            GET_EXECUTOR(R(1))
            POP(R(2))// key
            POP(R(3))// table
            GET_TEMPORARY(R(4))
            jit_prepare(C);
            jit_putargr(C, R(1));
            jit_putargr(C, R(3));
            jit_putargr(C, R(2));
            jit_putargr(C, R(4));
            jit_call(C, get_myjit_wrapper);
            PUSH(R(4))// push get return value on stack
            break;


        CASE(b_function_1)
            GET_EXECUTOR(R(1))
            PUSH_NULL(R(2))
            jit_prepare(C);
            jit_putargr(C, R(1));
            jit_putargr(C, R(2));
            jit_call(C, function_init);
            // R(3) is a pointer to Function struct
            jit_ldxi(C, R(3), R(2), FIELD_OFFSET(Object, fp), sizeof(Function*));
            // R(4) is a pointer to enclosing scope
            jit_addi(C, R(4), R(3), FIELD_OFFSET(Function, enclosing_scope));
            GET_SCOPE(R(5), R(1))
            MOVE_OBJECT(R(4), R(5))
            REFERENCE(R(4))
            // R(4) is a pointer to arguments_count
            jit_addi(C, R(4), R(3), FIELD_OFFSET(Function, arguments_count));
            jit_movi(C, R(5), instruction->function_argument.arguments_count);
            jit_str(C, R(4), R(5), sizeof(uint));
            // R(4) is a pointer to variadic
            jit_addi(C, R(4), R(3), FIELD_OFFSET(Function, variadic));
            jit_movi(C, R(5), instruction->function_argument.is_variadic);
            jit_str(C, R(4), R(5), sizeof(bool));
            // R(4) is a pointer to ftype
            jit_addi(C, R(4), R(3), FIELD_OFFSET(Function, ftype));
            jit_movi(C, R(5), f_bytecode);
            jit_str(C, R(4), R(5), sizeof(FunctionType));
            break;
        #define GET_PROGRAM(register) \
            jit_getarg(C, register, 1);
        CASE(b_function_2)
            // get sub_program
            // R(1) is pointer to current bytecode program
            GET_PROGRAM(R(1))
            // R(2) points to the first sub_program, R(2)=program->sub_programs[0]
            jit_ldxi(C, R(2), R(1), FIELD_OFFSET(BytecodeProgram, sub_programs), sizeof(BytecodeProgram*));
            // R(2) is moved by uint_argument
            jit_ldxi(C, R(2), R(2), instruction->uint_argument*sizeof(BytecodeProgram*), sizeof(BytecodeProgram*));

            INDEX_STACK(R(3), 0)
            // R(4) is a pointer to Function struct
            jit_ldxi(C, R(4), R(3), FIELD_OFFSET(Object, fp), sizeof(Function*));
            // R(4) is a pointer to source_pointer field
            jit_addi(C, R(4), R(4), FIELD_OFFSET(Function, source_pointer));
            jit_str(C, R(4), R(2), sizeof(BytecodeProgram*));
            // reference the program
            
            CATCH(R(3))
            jit_prepare(C);
            jit_putargr(C, R(2));
            jit_call(C, heap_object_reference);
            break;
        CASE(b_binary)
            GET_EXECUTOR(R(1))
            // R(2) is operator name
            POP(R(2))
            POP(R(3))
            POP(R(4))
            // R(5) will contain text field of operator object
            jit_ldxi(C, R(5), R(2), FIELD_OFFSET(Object, text), sizeof(char*));
            GET_TEMPORARY(R(6))
            jit_prepare(C);
            jit_putargr(C, R(1));
            jit_putargr(C, R(3));
            jit_putargr(C, R(4));
            jit_putargr(C, R(5));
            jit_putargr(C, R(6));
            jit_call(C, operator_myjit_wrapper);
            PUSH(R(6))
            break;
        CASE(b_prefix)
            GET_EXECUTOR(R(1))
            POP(R(2))
            POP(R(3))
            GET_TEMPORARY(R(4))
            jit_prepare(C);
            jit_putargr(C, R(1));
            jit_putargr(C, (jit_value)&null_const);
            jit_putargr(C, R(3));
            jit_putargi(C, R(2));
            jit_putargr(C, R(4));
            jit_call(C, operator_myjit_wrapper);
            PUSH(R(4))
            break;
        // TODO: implement each of these in myjit
        #define BINARY(instruction, operator) \
            CASE(instruction) \
                GET_EXECUTOR(R(1)) \
                POP(R(2)) \
                POP(R(3)) \
                GET_TEMPORARY(R(4)) \
                jit_prepare(C); \
                jit_putargr(C, R(1)); \
                jit_putargr(C, R(2)); \
                jit_putargr(C, R(3)); \
                jit_putargi(C, operator); \
                jit_putargr(C, R(4)); \
                jit_call(C, operator_myjit_wrapper); \
                PUSH(R(4)) \
                break;
        #define PREFIX(instruction, operator) \
            CASE(instruction) \
                GET_EXECUTOR(R(1)) \
                POP(R(2)) \
                GET_TEMPORARY(R(3)) \
                jit_prepare(C); \
                jit_putargr(C, R(1)); \
                jit_putargi(C, &null_const); \
                jit_putargr(C, R(2)); \
                jit_putargi(C, operator); \
                jit_putargr(C, R(3)); \
                jit_call(C, operator_myjit_wrapper); \
                PUSH(R(3)) \
                break;
        OPERATOR_INSTRUCTIONS
        #undef BINARY
        #undef PREFIX
        CASE(b_jump)
            // TODO: Jump back (although it isn't used in current version of bytecode generator)
            jit_labels[instruction->uint_argument]=jit_jmpi(C, JIT_FORWARD);
            break;
        CASE(b_jump_not)
            POP(R(1))
            jit_prepare(C);
            jit_putargr(C, R(1));
            jit_call(C, is_falsy_myjit_wrapper);
            jit_labels[instruction->uint_argument]=jit_beqi(C, JIT_FORWARD, R_OUT, true);
            break;
        CASE(b_label)
            jit_patch(C, jit_labels[instruction->uint_argument]);
            break;
        CASE(b_call)
        CASE(b_tail_call)
            GET_EXECUTOR(R(1))
            POP(R(2))
            INDEX_STACK(R(3), instruction->uint_argument)
            GET_TEMPORARY(R(4))
            jit_prepare(C);
            jit_putargr(C, R(1));
            jit_putargr(C, R(2));
            jit_putargr(C, R(3));
            jit_putargi(C, instruction->uint_argument);
            jit_putargr(C, R(4));
            jit_call(C, call_myjit_wrapper);
            DEREFERENCE(R(2), R(5))
            PUSH(R(4))
            if(instruction->type==b_call) {
                break;
            }
            // intentional fallthrough if instruction is of type b_tail_call
        CASE(b_end)
        CASE(b_return)
            // call pop, R(1)=vector_pop(OBJECT_STACK_REGISTER)
            jit_prepare(C);
            jit_putargr(C, OBJECT_STACK_REGISTER);
            jit_call(C, vector_pop);
            jit_retval(C, R(1));
            // the third argument of CompiledFunction is writeable pointer used to return Object struct
            // R(2)=result_argument
            jit_getarg(C, R(2), 2);
            // memcpy(R(2), R(1), sizeof(Object));
            jit_prepare(C);
            jit_putargr(C, R(2));
            jit_putargr(C, R(1));
            jit_putargi(C, sizeof(Object));
            jit_call(C, memcpy);
            jit_reti(C, 0);
            break;
        default:
            THROW_ERROR(BYTECODE_ERROR, "Don't know how to compile instruction %s.", INSTRUCTION_NAMES[instruction->type]);
            break;
    }
    #undef CASE
}

void compile_bytecode_program(Executor* E, BytecodeProgram* program){
    #ifdef HAS_MYJIT
    //creates a new instance of the compiler and ask it to assign result to program->compiled
    struct jit * C = jit_init();
    jit_prolog(C, &program->compiled);

    jit_declare_arg(C, JIT_PTR, sizeof(Executor*));
    jit_declare_arg(C, JIT_PTR, sizeof(BytecodeProgram*));
    jit_declare_arg(C, JIT_PTR, sizeof(Object*));

    // allocate space for one object on the stack frame, it is used by some operations
    int temporary_offset=jit_allocai(C, sizeof(Object));

    jit_getarg(C, R(1), 0);// get executor to R1
    jit_addi(C, OBJECT_STACK_REGISTER, R(1), (long unsigned)&E->stack-(long unsigned)E);// get object_stack to OBJECT_STACK_REGISTER
    
    jit_prepare(C);
    jit_putargi(C, "<Executing MyJIT function>\n");
    jit_call(C, printf);

    jit_op** jit_labels=malloc(sizeof(jit_op*)*program->labels_count);
    for(int i=0; true; i++) {
        instruction_to_myjit(E, C, &program->code[i], temporary_offset, jit_labels);
        if(program->code[i].type==b_end){
            break;
        }
    }
    free(jit_labels);

    jit_generate_code(C);
    // jit_dump_ops(C, JIT_DEBUG_OPS);
    // jit_check_code(C, JIT_WARN_ALL);

    jit_free(C);
    #endif
}