#include "bytecode.h"

char* INSTRUCTION_NAMES[]={
    #define X(t) #t,
    INSTRUCTION_TYPES
    #undef X
};

void stringify_instruction(const BytecodeProgram* prog, char* destination, Instruction instr, int buffer_count){
    #define STRINGIFY_BOOL(b) ((b) ? "true" : "false")
    switch(instr.type){
        case b_end:
        case b_discard:
        case b_get_scope:
        case b_set_scope:
        case b_null:
        case b_return:
        case b_binary:
        case b_prefix:
        case b_new_scope:
        case b_table_set:
        case b_table_set_keep:
        case b_get:
        case b_table_get:
            snprintf(destination, buffer_count, "%s\n", INSTRUCTION_NAMES[instr.type]);// these instructions doesn't use the argument
            break;
        case b_set:
            snprintf(destination, buffer_count, "%s %s\n", INSTRUCTION_NAMES[instr.type], STRINGIFY_BOOL(instr.bool_argument));
            break;
        case b_load_float:
            snprintf(destination, buffer_count, "%s %f\n", INSTRUCTION_NAMES[instr.type], instr.float_argument);
            break;
        case b_load_int:
            snprintf(destination, buffer_count, "%s %i\n", INSTRUCTION_NAMES[instr.type], instr.int_argument);
            break;
        case b_load_string:
            snprintf(destination, buffer_count, "%s %u \"%s\"\n", INSTRUCTION_NAMES[instr.type], instr.uint_argument, ((char*)prog->constants)+instr.uint_argument);
            break;
        case b_pre_function:
            snprintf(destination, buffer_count, "%s %s %u\n", INSTRUCTION_NAMES[instr.type], STRINGIFY_BOOL(instr.bool_argument), instr.pre_function_argument.arguments_count);
            break;
        default:
            snprintf(destination, buffer_count, "%s %u\n", INSTRUCTION_NAMES[instr.type], instr.uint_argument);
    }
    #undef STRINGIFY_BOOL
}

char* stringify_constants(const BytecodeProgram* prog){
    char* result=malloc(prog->constants_size);
    for(int i=0; i<prog->constants_size-1; i++){
        if(prog->constants[i]=='\0'){
            result[i]='\n';
        } else {
            result[i]=prog->constants[i];
        }
    }
    result[prog->constants_size]='\0';
    return result;
}

char* stringify_bytecode(const BytecodeProgram* prog){
    stream s;
    stream_init(&s, 64);

    int pointer=0;
    while(prog->code[pointer].type!=b_end){
        char line_number[8];
        snprintf(line_number, 8, "%i: ", pointer);

        stream_push_string(&s, line_number);

        char stringified_instruction[64];
        stringify_instruction(prog, (char*)&stringified_instruction, prog->code[pointer], 64);
        stream_push_string(&s, stringified_instruction);
        pointer++;
    }
    stream_push(&s, "CONSTANTS:", 10);
    for(int i=0; i<prog->constants_size-1;){
        char const_position[8];
        snprintf(const_position, 8, "\n%i: ", i);
        stream_push_string(&s, const_position);
        int old_position=s.position;
        stream_push_string(&s, prog->constants+i);
        i+=s.position-old_position+1;
    }
    stream_push(&s, "\n", 1);
    
    if(prog->sub_programs_count>0){
        for(int i=0; i<prog->sub_programs_count; i++){
            char buf[32];// assuming that the number of subprograms has less than 16 digits
            snprintf(buf, 32, "SUBPROGRAM[%i]\n", i);
            char* sub_program_stringified=stringify_bytecode(&prog->sub_programs[i]);
            stream_push(&s, buf, strlen(buf)*sizeof(char));
            stream_push(&s, sub_program_stringified, strlen(sub_program_stringified)*sizeof(char));
        }
    }
    stream_push(&s, "\0", sizeof(char));
    return stream_get_data(&s);
}

void bytecode_program_copy(const BytecodeProgram* source, BytecodeProgram* copy) {
    int c=0;
    for(; source->code[c].type!=b_end; c++);

    copy->code=malloc(sizeof(Instruction)*c);
    memcpy(copy->code, source->code, c);
    copy->information=malloc(sizeof(InstructionInformation)*c);
    memcpy(copy->information, source->information, c);
    copy->constants=malloc(source->constants_size);
    memcpy(copy->constants, source->constants, source->constants_size);
    copy->constants_size=source->constants_size;
    copy->labels=NULL;
    
    copy->sub_programs_count=source->sub_programs_count;
    copy->sub_programs=malloc(sizeof(BytecodeProgram)*copy->sub_programs_count);
    for(int i=0; i<source->sub_programs_count; i++){
       bytecode_program_copy(&source->sub_programs[i], &copy->sub_programs[i]);
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

void list_program_labels(BytecodeProgram* program){
    program->labels=list_labels(program->code);
    for(int i=0; i<program->sub_programs_count; i++){
        list_program_labels(&program->sub_programs[i]);
    }
}

void bytecode_program_destructor(Executor* E, BytecodeProgram* program) {
    for(int i=0; i<program->sub_programs_count; i++){
        gc_object_dereference(E, (gc_Object*)&program->sub_programs[i]);
    }
    bytecode_program_free(program);
}

void bytecode_program_init(Executor* E, BytecodeProgram* program){
    gc_pointer_init(E, (gc_Pointer*)&program->gcp, (gc_PointerDestructorFunction)bytecode_program_destructor);
    for(int i=0; i<program->sub_programs_count; i++){
        gc_pointer_init(E, (gc_Pointer*)&program->sub_programs[i], (gc_PointerDestructorFunction)bytecode_program_destructor);
        gc_object_reference((gc_Object*)&program->sub_programs[i]);
    }
}

void bytecode_program_free(BytecodeProgram* program) {
    free(program->labels);
    free(program->code);
    free(program->information);
    free(program->constants);
    free(program);
}

#define X(t, result) case t: return result;
int gets_from_stack(Instruction instr){
    switch(instr.type){
        X(b_discard, 1)
        //X(b_move_top, 1)
        X(b_return, 1)
        X(b_set_scope, 1)
        X(b_get, 1)
        X(b_table_get, 2)
        X(b_set, 2)
        X(b_table_set, 3)
        X(b_table_set_keep, 3)
        X(b_call, instr.uint_argument+1)
        X(b_binary, 3)
        X(b_prefix, 2)
        default: return 0;
    }
}
#undef X

#define X(i) || instr==i
bool pushes_to_stack(InstructionType instr){
    // the following instructions DON'T push to the stack
    return !(false
    X(b_move_top)
    X(b_push_to_top)
    X(b_end)
    X(b_no_op)
    X(b_discard)
    X(b_get_scope)
    X(b_set_scope)
    X(b_label)
    X(b_jump)
    X(b_jump_not)
    );
}
bool changes_flow(InstructionType instr){
    return false
    X(b_return)
    X(b_label)
    X(b_jump)
    X(b_jump_not);
}
bool changes_scope(InstructionType instr){
    return false
    X(b_return)
    X(b_new_scope)
    X(b_get_scope)
    X(b_set_scope);
}
#undef X