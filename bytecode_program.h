#ifndef BYTECODE_PROGRAM_H
#define BYTECODE_PROGRAM_H

#include "bytecode.h"

typedef struct {
    enum Type {
        a_type,
        a_constant
    } assumption_type;
    union {
        ObjectType type;
        Object constant;
    };
} Assumption;

#define CALLS_UNTIL_STATISTICS_MODE 10
#define REMEMBERED_CALLS 10
#define CONSTANT_THRESHOLD 3
typedef struct {
    Assumption* previous_calls[REMEMBERED_CALLS];
    Object* last_call;
    unsigned* constant_streaks;
} CallStatistics;

typedef struct FunctionVariant FunctionVariant;

typedef struct BytecodeProgram BytecodeProgram;
struct BytecodeProgram {
    ManagedPointer mp;
    
    char* source_file_name;
    Instruction* code;
    int* labels;
    InstructionInformation* information;
    char* constants;
    int constants_size;

    BytecodeProgram** sub_programs;
    int sub_programs_count;
    
    Assumption* assumptions;
    unsigned expected_arguments;
    unsigned* upvalues;
    unsigned upvalues_count;
    unsigned calls_count;
    CallStatistics* statistics;
    vector variants;
};

void print_instruction(Instruction instruction, void* constants);
void print_bytecode_program(const BytecodeProgram* program);
void bytecode_program_init(Executor* E, BytecodeProgram* program);
void bytecode_program_free(BytecodeProgram* program);
void list_program_labels(BytecodeProgram* program);
void bytecode_program_copy(const BytecodeProgram* source, BytecodeProgram* copy);
bool instructions_equal(Instruction a, Instruction b, void* constants);

#endif