#include "bytecode_program.h"

bool instructions_equal(Instruction a, Instruction b, void* constants){
    if(a.type!=b.type){
        return false;
    }
    switch(a.type){
        case b_end:
        case b_discard:
        case b_enter_scope:
        case b_leave_scope:
        case b_null:
        case b_return:
        case b_binary:
        case b_prefix:
        case b_new_scope:
        case b_table_set:
        case b_table_set_keep:
        case b_get:
        case b_table_get:
            return true;// these instructions doesn't use the argument
        case b_set:
            return a.bool_argument==b.bool_argument;
        case b_load_float:
            return a.float_argument==b.float_argument;
        case b_load_int:
            return a.int_argument==b.int_argument;
        case b_load_string:
            return a.uint_argument==b.uint_argument||strcmp((char*)constants+a.uint_argument, (char*)constants+b.uint_argument)==0;
        case b_function_1:
            return a.function_argument.is_variadic==b.function_argument.is_variadic
            &&     a.function_argument.arguments_count==b.function_argument.arguments_count;
        case b_swap:
            return a.swap_argument.left==b.swap_argument.left
            &&     a.swap_argument.right==b.swap_argument.right;
        default:
            return a.uint_argument==b.uint_argument;
    }
}

char* stringify_constants(const BytecodeProgram* program){
    char* result=malloc(program->constants_size);
    for(int i=0; i<program->constants_size-1; i++){
        if(program->constants[i]=='\0'){
            result[i]='\n';
        } else {
            result[i]=program->constants[i];
        }
    }
    result[program->constants_size]='\0';
    return result;
}

void print_instruction(Instruction instruction, void* constants){
    #define STRINGIFY_BOOL(b) ((b) ? "true" : "false")
    switch(instruction.type){
        case b_end:
        case b_discard:
        case b_enter_scope:
        case b_leave_scope:
        case b_null:
        case b_return:
        case b_binary:
        case b_prefix:
        case b_new_scope:
        case b_table_set:
        case b_table_set_keep:
        case b_get:
        case b_table_get:
            printf("%s", INSTRUCTION_NAMES[instruction.type]);// these instructions doesn't use the argument
            break;
        case b_set:
            printf("%s %s", INSTRUCTION_NAMES[instruction.type], STRINGIFY_BOOL(instruction.bool_argument));
            break;
        case b_load_float:
            printf("%s %f", INSTRUCTION_NAMES[instruction.type], instruction.float_argument);
            break;
        case b_load_int:
            printf("%s %i", INSTRUCTION_NAMES[instruction.type], instruction.int_argument);
            break;
        case b_load_string:
            printf("%s %u \"%s\"", INSTRUCTION_NAMES[instruction.type], instruction.uint_argument, ((char*)constants)+instruction.uint_argument);
            break;
        case b_function_1:
            printf("%s %s %u", INSTRUCTION_NAMES[instruction.type], STRINGIFY_BOOL(instruction.function_argument.is_variadic), instruction.function_argument.arguments_count);
            break;
        case b_swap:
            printf("%s %u %u", INSTRUCTION_NAMES[instruction.type], instruction.swap_argument.left, instruction.swap_argument.right);
            break;
        default:
            printf("%s %u", INSTRUCTION_NAMES[instruction.type], instruction.uint_argument);
    }
    #undef STRINGIFY_BOOL
}

void print_bytecode_program(const BytecodeProgram* program){
    int pointer=0;
    while(program->code[pointer].type!=b_end){
        printf("%i: ", pointer);

        print_instruction(program->code[pointer], program->constants);
        /*int comment=program->information[pointer].comment;
        if(comment>=0 && comment<program->constants_size){
            printf(" # %s\n", &program->constants[comment]);
        } else {*/
            printf("\n");
        //}
        pointer++;
    }
    printf("CONSTANTS:");
    for(int i=0; i<program->constants_size-1;){
        printf("\n%i: ", i);
        i+=printf("%s", program->constants+i)+1;// printf returns number of characters printed, we also skip the '\0' character
    }
    printf("\n");
    
    if(program->sub_programs_count>0){
        for(int i=0; i<program->sub_programs_count; i++){
            printf("SUBPROGRAM[%i]\n", i);
            print_bytecode_program(program->sub_programs[i]);
        }
    }
}

Assumption assumption_copy(Executor* E, Assumption source){
    Assumption copied={source.type};
    if(source.assumption_type==a_constant){
        copied.constant=copy(E, source.constant);
    } else {
        copied.type=source.type;
    }
    return copied;
}

BytecodeProgram* bytecode_program_copy(Executor* E, const BytecodeProgram* source, bool copy_assumptions) {
    BytecodeProgram* copy=malloc(sizeof(BytecodeProgram));
    int c=0;
    for(; source->code[c].type!=b_end; c++);
    c++;

    copy->code=malloc(sizeof(Instruction)*c);
    memcpy(copy->code, source->code, sizeof(Instruction)*c);
    copy->information=malloc(sizeof(InstructionInformation)*c);
    memcpy(copy->information, source->information, sizeof(InstructionInformation)*c);
    copy->constants=malloc(source->constants_size);
    memcpy(copy->constants, source->constants, source->constants_size);
    copy->constants_size=source->constants_size;
    memcpy(copy->upvalues, source->upvalues, source->upvalues_count*sizeof(unsigned));
    copy->upvalues_count=source->upvalues_count;
    copy->expected_arguments=source->expected_arguments;

    copy->calls_count=0;
    copy->statistics.initialized=false;
    if(copy_assumptions && source->assumptions!=NULL){
        uint assumptions_count=copy->upvalues_count+copy->expected_arguments;
        copy->assumptions=malloc(sizeof(Assumption)*assumptions_count);
        for(int i=0; i<assumptions_count; i++){
            copy->assumptions[i]=assumption_copy(E, source->assumptions[i]);
        }
    } else {
        copy->assumptions=NULL;
    }
    
    copy->sub_programs_count=source->sub_programs_count;
    copy->sub_programs=malloc(sizeof(BytecodeProgram)*copy->sub_programs_count);
    for(int i=0; i<source->sub_programs_count; i++){
       copy->sub_programs[i]=bytecode_program_copy(E, source->sub_programs[i], true);
    }
    bytecode_program_init(E, copy);
    return copy;
}

 BytecodeProgram* bytecode_program_copy_override(Executor* E, const BytecodeProgram* source){
     return bytecode_program_copy(E, source, true);
 }

#define INITIAL_LABELS_COUNT 4
void bytecode_program_list_labels(BytecodeProgram* program){
    uint* labels=malloc(INITIAL_LABELS_COUNT*sizeof(uint));
    int labels_count=INITIAL_LABELS_COUNT;
    int pointer=0;
    while(program->code[pointer].type!=b_end){
        if(program->code[pointer].type==b_label){
            while(program->code[pointer].uint_argument>labels_count){
                labels_count*=2;
                labels=realloc(labels, labels_count*sizeof(uint));
            }
            labels[program->code[pointer].uint_argument]=pointer;
        }
        pointer++;
    }
    program->labels=labels;
    program->labels_count=labels_count;
}
#undef INITIAL_LABELS_COUNT

void bytecode_program_count_stack_depth(BytecodeProgram* program){
    int max_stack_depth=0;
    int current_stack_depth=0;
    for(Instruction* i=program->code; i->type!=b_end; i++){
        current_stack_depth+=pushes_to_stack(*i);
        if(current_stack_depth>max_stack_depth){
            max_stack_depth=current_stack_depth;
        }
        current_stack_depth-=gets_from_stack(*i);
    }
    program->stack_depth=max_stack_depth;
}

void bytecode_program_foreach_children(Executor* E, BytecodeProgram* program, ManagedPointerForeachChildrenCallback callback) {
    for(int i=0; i<program->sub_programs_count; i++){
        Object wrapped=wrap_heap_object((HeapObject*)program->sub_programs[i]);
        callback(E, &wrapped);
    }
    for(int i=0; i<vector_count(&program->variants); i++){
        Object wrapped=wrap_heap_object((HeapObject*)pointers_vector_get(&program->variants, i));
        callback(E, &wrapped);
    }
}

void bytecode_program_free(BytecodeProgram* program) {
    free(program->source_file_name);
    free(program->labels);
    free(program->code);
    free(program->information);
    free(program->constants);
    free(program->upvalues);
    vector_deinit(&program->variants);
    free(program);
}

void bytecode_program_init(Executor* E, BytecodeProgram* program){
    bytecode_program_list_labels(program);
    bytecode_program_count_stack_depth(program);
    program->calls_count=0;
    program->statistics.initialized=false;
    program->assumptions=NULL;
    vector_init(&program->variants, sizeof(BytecodeProgram), 4);
    managed_pointer_init(E, (ManagedPointer*)&program->mp, (ManagedPointerFreeFunction)bytecode_program_free);
    program->mp.foreach_children=(ManagedPointerForeachChildrenFunction)bytecode_program_foreach_children;
    program->mp.copy=(ManagedPointerCopyFunction)bytecode_program_copy_override;
    for(int i=0; i<program->sub_programs_count; i++){
        program->sub_programs[i]->source_file_name=strdup(program->source_file_name);
        bytecode_program_init(E, program->sub_programs[i]);
        heap_object_reference((HeapObject*)program->sub_programs[i]);
    }
    program->compiled=NULL;
}