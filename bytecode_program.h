#ifndef BYTECODE_PROGRAM_H
#define BYTECODE_PROGRAM_H

#include "bytecode.h"
#include "c_fixes.h"

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

typedef struct {
    // CallStatistics take incredible amount of memory
    // so I decided to allocate them only when the calls are collected
    // and then deallocate them after a variant is generated
    bool initialized;
    Assumption** previous_calls;
    Object* last_call;
    unsigned* constant_streaks;
    uint collected_calls;
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
    CallStatistics statistics;
    vector variants;
};

void print_instruction(Instruction instruction, void* constants);
void print_bytecode_program(const BytecodeProgram* program);
void bytecode_program_init(Executor* E, BytecodeProgram* program);
void bytecode_program_free(BytecodeProgram* program);
void list_program_labels(BytecodeProgram* program);
BytecodeProgram* bytecode_program_copy(Executor* E, const BytecodeProgram* source, bool copy_assumptions);
bool instructions_equal(Instruction a, Instruction b, void* constants);

#endif