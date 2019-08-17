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
        i+=printf(program->constants+i)+1;// printf returns number of characters printed, we also skip the '\0' character
    }
    printf("\n");
    
    if(program->sub_programs_count>0){
        for(int i=0; i<program->sub_programs_count; i++){
            printf("SUBPROGRAM[%i]\n", i);
            print_bytecode_program(program->sub_programs[i]);
        }
    }
}

void bytecode_program_copy(const BytecodeProgram* source, BytecodeProgram* copy) {
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
    copy->assumptions=NULL;
    
    copy->sub_programs_count=source->sub_programs_count;
    copy->sub_programs=malloc(sizeof(BytecodeProgram)*copy->sub_programs_count);
    for(int i=0; i<source->sub_programs_count; i++){
       bytecode_program_copy(source->sub_programs[i], copy->sub_programs[i]);
    }
}

#define INITIAL_LABELS_COUNT 4
int* list_labels(Instruction* code){
    int* labels=malloc(INITIAL_LABELS_COUNT*sizeof(int));
    int labels_count=INITIAL_LABELS_COUNT;
    int pointer=0;
    while(code[pointer].type!=b_end){
        if(code[pointer].type==b_label){
            while(code[pointer].uint_argument>labels_count){
                labels_count*=2;
                labels=realloc(labels, labels_count*sizeof(int));
            }
            labels[code[pointer].uint_argument]=pointer;
        }
        pointer++;
    }
    return labels;
}

void bytecode_program_foreach_children(Executor* E, BytecodeProgram* program, ManagedPointerForeachChildrenCallback callback) {
    for(int i=0; i<program->sub_programs_count; i++){
        Object wrapped=wrap_heap_object((HeapObject*)program->sub_programs[i]);
        callback(E, &wrapped);
    }
}

void bytecode_program_free(BytecodeProgram* program) {
    free(program->source_file_name);
    free(program->labels);
    free(program->code);
    free(program->information);
    free(program->constants);
    free(program);
}

void bytecode_program_init(Executor* E, BytecodeProgram* program){
    program->labels=list_labels(program->code);
    program->calls_count=0;
    program->statistics.initialized=false;
    program->assumptions=NULL;
    vector_init(&program->variants, sizeof(BytecodeProgram), 4);
    managed_pointer_init(E, (ManagedPointer*)&program->mp, (ManagedPointerFreeFunction)bytecode_program_free);
    program->mp.foreach_children=(ManagedPointerForeachChildrenFunction)bytecode_program_foreach_children;
    for(int i=0; i<program->sub_programs_count; i++){
        program->sub_programs[i]->source_file_name=strdup(program->source_file_name);
        bytecode_program_init(E, program->sub_programs[i]);
        heap_object_reference((HeapObject*)program->sub_programs[i]);
    }
}