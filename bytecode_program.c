#include "bytecode_program.h"

void stringify_instruction(const BytecodeProgram* program, char* destination, Instruction instr, int buffer_count){
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
            snprintf(destination, buffer_count, "%s", INSTRUCTION_NAMES[instr.type]);// these instructions doesn't use the argument
            break;
        case b_set:
            snprintf(destination, buffer_count, "%s %s", INSTRUCTION_NAMES[instr.type], STRINGIFY_BOOL(instr.bool_argument));
            break;
        case b_load_float:
            snprintf(destination, buffer_count, "%s %f", INSTRUCTION_NAMES[instr.type], instr.float_argument);
            break;
        case b_load_int:
            snprintf(destination, buffer_count, "%s %i", INSTRUCTION_NAMES[instr.type], instr.int_argument);
            break;
        case b_load_string:
            snprintf(destination, buffer_count, "%s %u \"%s\"", INSTRUCTION_NAMES[instr.type], instr.uint_argument, ((char*)program->constants)+instr.uint_argument);
            break;
        case b_pre_function:
            snprintf(destination, buffer_count, "%s %s %u", INSTRUCTION_NAMES[instr.type], STRINGIFY_BOOL(instr.pre_function_argument.is_variadic), instr.pre_function_argument.arguments_count);
            break;
        case b_swap:
            snprintf(destination, buffer_count, "%s %u %u", INSTRUCTION_NAMES[instr.type], instr.swap_argument.left, instr.swap_argument.right);
            break;
        default:
            snprintf(destination, buffer_count, "%s %u", INSTRUCTION_NAMES[instr.type], instr.uint_argument);
    }
    #undef STRINGIFY_BOOL
}

bool instructions_equal(BytecodeProgram* program, Instruction a, Instruction b){
    if(a.type!=b.type){
        return false;
    }
    switch(a.type){
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
            return true;// these instructions doesn't use the argument
        case b_set:
            return a.bool_argument==b.bool_argument;
        case b_load_float:
            return a.float_argument==b.float_argument;
        case b_load_int:
            return a.int_argument==b.int_argument;
        case b_load_string:
            return a.uint_argument==b.uint_argument||strcmp(((char*)program->constants)+a.uint_argument, ((char*)program->constants)+b.uint_argument)==0;
        case b_pre_function:
            return a.pre_function_argument.is_variadic==b.pre_function_argument.is_variadic
            &&     a.pre_function_argument.arguments_count==b.pre_function_argument.arguments_count;
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

char* stringify_bytecode(const BytecodeProgram* program){
    stream s;
    stream_init(&s, 64);

    int pointer=0;
    while(program->code[pointer].type!=b_end){
        char line_number[8];
        snprintf(line_number, 8, "%i: ", pointer);

        stream_push_string(&s, line_number);

        char stringified_instruction[64];
        stringify_instruction(program, (char*)&stringified_instruction, program->code[pointer], 64);
        stream_push_string(&s, stringified_instruction);
        /*int comment=program->information[pointer].comment;
        if(comment>=0 && comment<program->constants_size){
            stream_printf(&s, " # %s\n", &program->constants[comment]);
        } else {*/
            stream_push_const_string(&s, "\n");
        //}
        pointer++;
    }
    stream_push(&s, "CONSTANTS:", 10);
    for(int i=0; i<program->constants_size-1;){
        char const_position[8];
        snprintf(const_position, 8, "\n%i: ", i);
        stream_push_string(&s, const_position);
        int old_position=s.position;
        stream_push_string(&s, program->constants+i);
        i+=s.position-old_position+1;
    }
    stream_push(&s, "\n", 1);
    
    if(program->sub_programs_count>0){
        for(int i=0; i<program->sub_programs_count; i++){
            char buf[32];// assuming that the number of subprograms has less than 16 digits
            snprintf(buf, 32, "SUBPROGRAM[%i]\n", i);
            char* sub_program_stringified=stringify_bytecode(&program->sub_programs[i]);
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
    copy->statistics=NULL;
    copy->assumptions=NULL;
    
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

void bytecode_program_foreach_children(Executor* E, BytecodeProgram* program, gc_PointerForeachChildrenCallback callback) {
    program->gcp.gco.marked=true;
    for(int i=0; i<program->sub_programs_count; i++){
        Object wrapped=wrap_gc_object((gc_Object*)&program->sub_programs[i]);
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
    program->statistics=NULL;
    program->assumptions=NULL;
    vector_init(&program->variants, sizeof(BytecodeProgram), 4);
    gc_pointer_init(E, (gc_Pointer*)&program->gcp, (gc_PointerFreeFunction)bytecode_program_free);
    program->gcp.foreach_children=(gc_PointerForeachChildrenFunction)bytecode_program_foreach_children;
    for(int i=0; i<program->sub_programs_count; i++){
        program->sub_programs[i].source_file_name=strdup(program->source_file_name);
        bytecode_program_init(E, &program->sub_programs[i]);
        gc_object_reference((gc_Object*)&program->sub_programs[i]);
    }
}