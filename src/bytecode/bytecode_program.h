#ifndef BYTECODE_PROGRAM_H
#define BYTECODE_PROGRAM_H

#include "bytecode.h"
#include "../c_fixes.h"

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

typedef struct BytecodeProgram BytecodeProgram;

typedef void (*CompiledFunction)(Executor* E, BytecodeProgram* bytecode_program);

typedef struct FunctionVariant FunctionVariant;

struct BytecodeProgram {
    ManagedPointer mp;
    
    char* source_file_name;
    Instruction* code;
    uint* labels;
    uint labels_count;
    InstructionInformation* information;
    char* constants;
    int constants_size;

    uint stack_depth;

    BytecodeProgram** sub_programs;
    int sub_programs_count;
    
    Assumption* assumptions;
    uint expected_arguments;
    uint* upvalues;
    uint upvalues_count;
    uint calls_count;
    CallStatistics statistics;
    vector variants;
    CompiledFunction compiled;
};

void print_instruction(Instruction instruction, void* constants);
void print_bytecode_program(const BytecodeProgram* program);
void bytecode_program_init(Executor* E, BytecodeProgram* program);
void bytecode_program_free(BytecodeProgram* program);
void list_program_labels(BytecodeProgram* program);
BytecodeProgram* bytecode_program_copy(Executor* E, const BytecodeProgram* source, bool copy_assumptions);
bool instructions_equal(Instruction a, Instruction b, void* constants);

#endif