#include "bytecode_optimisations.h"

#define LOG_BYTECODE_OPTIMISATIONS 0
#define LOG_IF_ENABLED(message, ...) \
    if(LOG_BYTECODE_OPTIMISATIONS){ \
        printf(message, ##__VA_ARGS__); \
    }

bool is_path_part(instruction instr){
    return  instr.type==b_get || instr.type==b_table_get || instr.type==b_load_string;
}

int path_length(const instruction* code,  int path_start){
    if(code[path_start].type!=b_get && code[path_start].type!=b_set){
        return 0;// path must start with either set or get
    }
    int p=1;
    for(; is_path_part(code[path_start-p]) && p<path_start+1; p++);
    return p;
}

/*int path_length(const instruction* code,  int path_start){
    if(code[path_start].type!=b_get && code[path_start].type!=b_set){
        return 0;// path must start with either set or get
    }
    int balance=-1;
    if(code[path_start].type==b_set){
        balance--;// value assigned by set is not part of the path
    }
    for(int p=0; is_path_part(code[path_start-p]); p++){
        balance+=pushes_to_stack(code[p].type)-gets_from_stack(code[p]);
        if(balance==0){
            return p;
        }
    }
}*/

bool instructions_equal(instruction a, instruction b){
    return a.type==b.type && a.argument == b.argument;
}

bool paths_equal(const instruction* code, int path1, int path2){
    int p=0;
    while(is_path_part(code[path1+p]) && is_path_part(code[path2+p])){
        if(!instructions_equal(code[path1+p], code[path2+p])){
            return false;
        }
        p--;
    }
    return true;
}

int count_instructions(instruction* code){
    int p=0;
    for(; code[p].type!=b_end; p++);
    return p;
}

void move_instructions(bytecode_program* prog, int starting_index, int movement){
    int initial_count=count_instructions(prog->code);
    int moved_count=initial_count+movement;
    instruction* new_code=malloc((moved_count+1)*sizeof(instruction));

    //if(LOG_BYTECODE_OPTIMISATIONS)
    //    printf("move_instructions(%i, %i)\n", starting_index, movement);

    for(int i=0; i<moved_count; i++){
        if(i<=starting_index+movement){
            new_code[i]=prog->code[i];
        } else {
            new_code[i]=prog->code[i-movement];
        }
    }
    instruction end_instruction={b_end, 0};
    new_code[moved_count]=end_instruction;
    prog->code=new_code;
}

void highlight_instructions(bytecode_program* prog, char symbol, int start, int end){
    stream s;
    init_stream(&s, 64);

    int pointer=0;
    while(prog->code[pointer].type!=b_end){
        if(pointer>=start && pointer<=end){
            stream_push(&s, &symbol, 1);
            stream_push(&s, " ", 1);
        } else {
            stream_push(&s, "  ", 2);
        }
        char stringified_instruction[64];
        stringify_instruction(prog, (char*)&stringified_instruction, prog->code[pointer], 64);
        int stringified_length=strlen(stringified_instruction);

        stream_push(&s, stringified_instruction, stringified_length*sizeof(char));
        pointer++;
    }
    stream_push(&s, "\n\0", 2);
    printf(stream_get_data(&s));
    free(stream_get_data(&s));
}

void insert_instruction(bytecode_program* prog, int point, instruction instr){
    move_instructions(prog, point-1, 1);
    prog->code[point]=instr;
    if(LOG_BYTECODE_OPTIMISATIONS){
        highlight_instructions(prog, '+', point, point);
    }
}

void remove_instructions(bytecode_program* prog, int start, int end){
    move_instructions(prog, end, start-end-1);// remove get path instructions
}

void fill_with_no_op(bytecode_program* prog, int start, int end){
    if(LOG_BYTECODE_OPTIMISATIONS){
        highlight_instructions(prog, '-', start, end);
    }
    instruction instr={b_no_op, 0};
    
    for(int p=start; p<=end; p++){
        prog->code[p]=instr;
    }
}

/*
This function makes sure that the object that was on top of the stack before start
will be on top of the stack after end and all instructions between start and end
still work correctly
returns numbers of operations added
*/
int keep_stack_top_unchanged(bytecode_program* prog, int start, int end){
    int instructions_added=0;
    // count how many objects are pushed to the stack and how many are taken from it
    int counter=0;
    for(int pointer=start; pointer<=end; pointer++){
        int stack_top_postion=counter;
        counter-=gets_from_stack(prog->code[pointer]);
        // if the instruction takes more from the stack than was placed
        // before start, then we need to move the current stack top back,
        // because it certainly wants the values placed on stack before start point
        if(counter<0){
            while(counter<0){
                instruction push_to_top_instruction={b_push_to_top, stack_top_postion-counter};
                insert_instruction(prog, pointer, push_to_top_instruction);
                instructions_added++;
                // the instruction was added after the pointer, so it needs to be skipped
                // also the end needs to be moved to compensate for that
                pointer++;
                end++;
                counter++;
            }
            // nowthe items that were on top of the stack are beneath the ones pushed to the top
            // in the while block, so they need to be pushed back to the top so that the order
            // is preserved
            for(int i=0; i<stack_top_postion; i++){
                instruction push_to_top_instruction={b_push_to_top, stack_top_postion};
                insert_instruction(prog, pointer, push_to_top_instruction);
                instructions_added++;
                pointer++;
                end++;
            }
        }
        counter+=pushes_to_stack(prog->code[pointer].type);
    }
    if(counter>0){
        // add push_to_top instruction that moves the object that was on top of the stack
        // before start point back to the top
        instruction push_to_top_instruction={b_push_to_top, counter};
        insert_instruction(prog, end+1, push_to_top_instruction);
        instructions_added++;
    }
    return instructions_added;
}

void remove_no_ops(bytecode_program* prog){
    int block_start=0;
    bool inside_block=false;
    for(int p=0; prog->code[p].type!=b_end; p++){
        if(inside_block){
            if(prog->code[p].type!=b_no_op || prog->code[p+1].type==b_end){
                remove_instructions(prog, block_start, p-1);
                // move the pointer back by the number of instructions removed
                p=p-1-block_start;
                inside_block=false;
            }
        } else if(prog->code[p].type==b_no_op){
            block_start=p;
            inside_block=true;
        }
    }
}

void optimise_bytecode(bytecode_program* prog){
    for(int i=0; i<prog->sub_programs_count; i++){
        optimise_bytecode(&prog->sub_programs[i]);
    }
    if(LOG_BYTECODE_OPTIMISATIONS){
        USING_STRING(stringify_bytecode(prog),
            printf("Unoptimised bytecode:\n%s\n", str));
    }
    for(int pointer=count_instructions(prog->code); pointer>=0; pointer--){
        if(prog->code[pointer].type==b_set && prog->code[pointer+1].type==b_discard
           && !prog->code[pointer].argument /* argument tells whether the variable is used in closure, we can't tell if the closure changes the variable*/
           && path_length(prog->code, pointer)<=2){// don't optimise nested paths like table.key, only single name paths
            if(LOG_BYTECODE_OPTIMISATIONS){
                printf("Found a set instruction\n");
                highlight_instructions(prog, '>', pointer-path_length(prog->code, pointer)+1, pointer);
            }
            bool first_get_removal=true;
            bool used=false;
            for(int search_pointer=pointer+2; prog->code[search_pointer].type!=b_end; search_pointer++){
                if(changes_flow(prog->code[search_pointer].type) || prog->code[search_pointer].type==b_function){
                    // we can't tell if the variable is used later
                    used=true;
                    break;
                }
                if(changes_scope(prog->code[search_pointer].type)){
                    // we optimised all gets in this scope so the variable isn't needed anymore
                    break;
                }
                if(prog->code[search_pointer].type==b_get && paths_equal(prog->code, pointer-1, search_pointer-1)){
                    if(LOG_BYTECODE_OPTIMISATIONS){
                        printf("Found a corresponding get instruction\n");
                        highlight_instructions(prog, '>', search_pointer-path_length(prog->code, pointer)+1, search_pointer);
                    }
                    // for now only single name variables are optimised
                    int get_path_length=2;//path_length(prog->code, search_pointer);

                    if(first_get_removal){
                        LOG_IF_ENABLED("Removing discard instruction after set instruction:\n");
                        fill_with_no_op(prog, pointer+1, pointer+1);
                        first_get_removal=false;
                    } else{
                        instruction double_instruction={b_double, 0};
                        insert_instruction(prog, pointer+1, double_instruction);
                        search_pointer++;
                    }
                    search_pointer+=keep_stack_top_unchanged(prog, pointer+1, search_pointer-get_path_length);
                    fill_with_no_op(prog, search_pointer-get_path_length+1, search_pointer);

                    if(search_pointer>=count_instructions(prog->code)){
                        break;// after removing the path search_pointer points to b_end, if loop continued from this point it would get past the code's end
                    }
                }
            }
            if(!used){
                // the variable isn't used in it's own scope and in any closure, so it can be removed
                LOG_IF_ENABLED("Removing set instruction:\n");
                fill_with_no_op(prog, pointer-path_length(prog->code, pointer)+1, pointer);
            }
        }
    }
    remove_no_ops(prog);
}

#undef LOG_BYTECODE_OPTIMISATIONS
#undef LOG