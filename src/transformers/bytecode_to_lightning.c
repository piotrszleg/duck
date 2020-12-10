#include "bytecode_to_lightning.h"
#include "../execution/execution.h"

#ifdef HAS_LIGHTNING

static jit_state_t *_jit;

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

// used for inspecting objects in the debugger
int catch_in_gdb(Object* o){
   return o->int_value;
}

typedef jit_reg_t Register;

#define ERROR(...) { printf(__VA_ARGS__); exit(-1); }

Register pop_register(vector* v, char* error_message, int line){
    if(vector_empty(v))
        ERROR(error_message, line);
    return *(Register*)vector_pop(v);
}

#define REGISTERS_COUNT 3

void instruction_to_lightning(Executor* E, 
                          Instruction* instruction, 
                          vector* temporary_registers,
                          vector* preserved_registers,
                          int temporary_offset,
                          int executor_offset,
                          int program_offset, 
                          int stack_offset,
                          jit_node_t** jit_labels) {

    #define TEMPORARY_REGISTER() pop_register(temporary_registers, "Out of temporary registers. (%i)", __LINE__)
    #define PRESERVED_REGISTER() pop_register(preserved_registers, "Out of preserved registers. (%i)", __LINE__)

    #define CALL(function) \
                if(vector_count(temporary_registers)<REGISTERS_COUNT) { \
                    ERROR("Temporary registers in use at call. (%i)", __LINE__) \
                } else { \
                    jit_finishi(function); \
                }
    #define PRINT(message) \
        jit_prepare(); \
        jit_pushargi((long)message); \
        CALL(printf);
    #define RETURN_TEMPORARY_REGISTER(register) vector_push(temporary_registers, &register);
    #define RETURN_PRESERVED_REGISTER(register) vector_push(preserved_registers, &register);
    
    #define GET_TEMPORARY(register) \
        jit_addi(register, JIT_FP, temporary_offset);

    #define LOAD_LOCAL(register, offset) \
        jit_ldxi_l(register, JIT_FP, offset);
    
    #define GET_EXECUTOR(register) LOAD_LOCAL(register, executor_offset)
    #define GET_PROGRAM(register)  LOAD_LOCAL(register, program_offset)
    #define GET_STACK(register)    LOAD_LOCAL(register, stack_offset)

    #define MOVE_OBJECT(destination, source) \
    {   Register loaded=TEMPORARY_REGISTER(); \
        jit_ldr(loaded, source); \
        jit_str(destination, loaded); \
        RETURN_TEMPORARY_REGISTER(loaded);  }

    #define SET_STACK(_) ;

    #define POP(register) \
    {   Register stack=TEMPORARY_REGISTER(); \
        GET_STACK(stack); \
        jit_ldr(register, stack); \
        jit_subi(register, register, sizeof(Object)); \
        jit_str(stack, register); \
        RETURN_TEMPORARY_REGISTER(stack); }

    #define REFERENCE(register) \
        jit_prepare(); \
        jit_pushargr(register); \
        CALL(reference);

    #define DEREFERENCE(register, executor_temporary) \
        GET_EXECUTOR(executor_temporary); \
        jit_prepare(); \
        jit_pushargr(executor_temporary); \
        jit_pushargr(register); \
        CALL(dereference);

    #define PUSH(register) \
    {   Register stack=PRESERVED_REGISTER(); \
        GET_STACK(stack); \
        MOVE_OBJECT(stack, register) \
        jit_addi(stack, stack, sizeof(Object)); \
        SET_STACK(stack); \
        RETURN_PRESERVED_REGISTER(stack) \
        REFERENCE(register)   }

    #define INDEX_STACK(register, index) \
        GET_STACK(register); \
        if(index>0){ \
            jit_subi(register, register, (index)*sizeof(Object)); \
        } else { \
            jit_retval(register); \
        }

    #define CATCH(register) \
        jit_prepare(); \
        jit_pushargr(register); \
        CALL(catch_in_gdb);

    // E->bytecode_environment.object_stack
    #define PRINT_STACK(executor, stack_temporary) \
        jit_addi(stack_temporary, executor, FIELD_OFFSET(Executor, bytecode_environment)+FIELD_OFFSET(BytecodeEnvironment, object_stack)); \
        jit_prepare(); \
        jit_pushargr(executor); \
        jit_pushargr(stack_temporary); \
        CALL(print_object_stack);

    // pushes null to the stack and writes  pointer to it to the register
    // vector_push(OBJECT_STACK_REGISTER, null_const);
    // register=vector_top(OBJECT_STACK_REGISTER);
    #define PUSH_NULL(top) \
    {   Register null_register=TEMPORARY_REGISTER(); \
        jit_movi(null_register, (long int)&null_const); \
        Register stack=TEMPORARY_REGISTER(); \
        GET_STACK(stack); \
        jit_ldr(top, stack); \
        jit_addi(top, top, sizeof(Object)); \
        MOVE_OBJECT(top, null_register) \
        jit_str(stack, top); \
        RETURN_TEMPORARY_REGISTER(stack) \
        RETURN_TEMPORARY_REGISTER(null_register)  }

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
        CASE(b_null)
            Register discarded=PRESERVED_REGISTER();
            PUSH_NULL(discarded);
            RETURN_PRESERVED_REGISTER(discarded);
            break;
        CASE(b_swap)
            // *temporary=*INDEX_STACK(instruction->swap_argument.left)
            Register temporary=PRESERVED_REGISTER();
            GET_TEMPORARY(temporary)
            Register left=PRESERVED_REGISTER();
            INDEX_STACK(left, instruction->swap_argument.left)
            MOVE_OBJECT(temporary, left)
            // *INDEX_STACK(instruction->swap_argument.left)=*INDEX_STACK(instruction->swap_argument.right)
            Register right=PRESERVED_REGISTER();
            INDEX_STACK(right, instruction->swap_argument.right)
            MOVE_OBJECT(left, right)
            // *INDEX_STACK(instruction->swap_argument.right)=*temporary
            MOVE_OBJECT(right, temporary)
            RETURN_PRESERVED_REGISTER(temporary);
            RETURN_PRESERVED_REGISTER(left);
            RETURN_PRESERVED_REGISTER(right);
            break;
        // TODO: implement each of these directly
        #define BINARY(instruction, operator) \
            CASE(instruction) { \
                Register executor=TEMPORARY_REGISTER(); \
                GET_EXECUTOR(executor) \
                Register left=TEMPORARY_REGISTER(); \
                POP(left) \
                Register right=TEMPORARY_REGISTER(); \
                POP(right) \
                Register result=PRESERVED_REGISTER(); \
                GET_TEMPORARY(result) \
                jit_prepare(); \
                jit_pushargr(executor); \
                RETURN_TEMPORARY_REGISTER(executor); \
                jit_pushargr(left); \
                RETURN_TEMPORARY_REGISTER(left); \
                jit_pushargr(right); \
                RETURN_TEMPORARY_REGISTER(right); \
                jit_pushargi((long)operator); \
                jit_pushargr(result); \
                CALL(operator_lightning_wrapper); \
                /*PUSH(result)*/ \
                RETURN_PRESERVED_REGISTER(result); \
                break; \
            }
        #define PREFIX(instruction, operator) \
            CASE(instruction) { \
                GET_EXECUTOR(JIT_V1) \
                POP(JIT_V2) \
                GET_TEMPORARY(JIT_V3) \
                jit_prepare(); \
                jit_pushargr(JIT_V1); \
                jit_pushargi((long)&null_const); \
                jit_pushargr(JIT_V2); \
                jit_pushargi((long)operator); \
                jit_pushargr(JIT_V3); \
                CALL(operator_lightning_wrapper); \
                PUSH(JIT_V3) \
                break; \
            }
        OPERATOR_INSTRUCTIONS
        #undef BINARY
        #undef PREFIX
        CASE(b_discard)
            Register popped=PRESERVED_REGISTER();
            // popped=vector_pop(OBJECT_STACK_REGISTER);
            POP(popped)
            // JIT_V2=E;
            // DEREFERENCE(JIT_V1, JIT_V2)
            RETURN_PRESERVED_REGISTER(popped);
            break;
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

void initialize_lightning_function(
    int executor_offset,
    int program_offset, 
    int stack_offset,
    vector* temporary_registers, 
    vector* preserved_registers) {

    // write register to memory allocated for the function at offset
    #define WRITE_TO_ALLOCATED(value_register, offset) { \
        Register allocated=TEMPORARY_REGISTER(); \
        jit_addi(allocated, JIT_FP, offset); \
        jit_str_l(allocated, value_register); \
        RETURN_TEMPORARY_REGISTER(allocated) \
    }

    // get argument to argument_register and then write it to memory
    #define WRITE_ARGUMENT(argument_register, offset) \
        jit_getarg(argument_register, jit_arg()); \
        WRITE_TO_ALLOCATED(argument_register, offset)

    // retrieve arguments and assign the three local variables located at given offsets

    Register executor=TEMPORARY_REGISTER();
    WRITE_ARGUMENT(executor, executor_offset)

    Register stack=executor;
    jit_addi(stack, stack, FIELD_OFFSET(Executor, stack));
    jit_addi(stack, stack, FIELD_OFFSET(Stack, top));
    WRITE_TO_ALLOCATED(stack, stack_offset)
    RETURN_TEMPORARY_REGISTER(stack);

    Register program=TEMPORARY_REGISTER();
    WRITE_ARGUMENT(program, program_offset)
    RETURN_TEMPORARY_REGISTER(program);

    PRINT("-- lightning function start\n")
}

void compile_bytecode_program(Executor* E, BytecodeProgram* program){
    printf("Started assembling.\n");
    
    //creates a new instance of the compiler and ask it to assign result to program->compiled
    init_jit(program->source_file_name);
    _jit = jit_new_state();

    jit_note(__FILE__, __LINE__);
    jit_prolog();
    
    // allocate space for one object on the stack frame, it is used by some operations
    int temporary_offset=jit_allocai(sizeof(Object));

    int executor_offset=jit_allocai(sizeof(Executor*));
    int program_offset=jit_allocai(sizeof(BytecodeProgram*));
    int stack_offset=jit_allocai(sizeof(Object*));

    #define ADD(vector, register) { \
        Register pushed=register; \
        vector_push(&vector, &pushed); \
    }

    vector temporary_registers;
    vector_init(&temporary_registers, sizeof(Register), 3);
    ADD(temporary_registers, JIT_R0)
    ADD(temporary_registers, JIT_R1)
    ADD(temporary_registers, JIT_R2)
    vector preserved_registers;
    vector_init(&preserved_registers, sizeof(Register), 3);
    ADD(preserved_registers, JIT_V0)
    ADD(preserved_registers, JIT_V1)
    ADD(preserved_registers, JIT_V2)
    
    initialize_lightning_function(
        executor_offset,
        program_offset,
        stack_offset,
        &temporary_registers,
        &preserved_registers
    );

    jit_node_t** jit_labels=malloc(sizeof(jit_node_t*)*program->labels_count);
    for(int i=0; true; i++) {
        instruction_to_lightning(E, 
                            &program->code[i],
                            &temporary_registers,
                            &preserved_registers, 
                            temporary_offset,
                            executor_offset, 
                            program_offset,
                            stack_offset,
                            jit_labels);
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

    printf("Finished assembling.\n");

    program->compiled=jit_emit();

    //jit_destroy_state();
    finish_jit();

    printf("Finished compiling.\n");
}
#else
void compile_bytecode_program(Executor* E, BytecodeProgram* program){}

#endif