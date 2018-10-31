#include "parser/parser.h"
#include "object_system/error.h"
#include "bytecode.h"

typedef void (*bytecode_test_function)(instruction*);

int instructions_equal(instruction a, instruction b){
    return a.type==b.type && a.argument==b.argument;
}

void assert_instruction_streams_equal(instruction* code1, instruction* code2){
    int pointer=0;
    int string_end=0;
    while(code1[pointer].type!=b_end && code2[pointer].type!=b_end){
        assert(instructions_equal(code1[pointer], code2[pointer]));
    }
    assert(code1[pointer].type!=b_end==code2[pointer].type);// tests whether both streams ended
}

void test_wrapper(bytecode_test_function test, const char* code_string){
    parse_string(code_string);

    instruction* bytecode=ast_to_bytecode(parsing_result, 1);
    test(bytecode);
    free(bytecode);

    delete_expression(parsing_result);
    delete_expression(parsing_result);
}

void literals(instruction* instr){
    printf("TEST: %s\n", __FUNCTION__);

    assert(instr->argument=0);
    instruction expected[]={{b_load_number, 0}, {b_load_string, 0}};
    assert_instruction_streams_equal(instr, expected);

    printf("test successful\n");
}

int main(int argc){
    parse_string("6+\"John\"+a.b.c");
    //parse_file(argv[1]);
    TRY_CATCH(
        test_wrapper(literals, "1, 2");
    ,
        printf(err_message);
        exit(-1);
    )
}