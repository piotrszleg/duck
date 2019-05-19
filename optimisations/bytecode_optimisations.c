#include "bytecode_optimisations.h"

#define LOG_BYTECODE_OPTIMISATIONS 1
#define LOG_CHANGE(message) \
    if(LOG_BYTECODE_OPTIMISATIONS){ \
        USING_STRING(stringify_bytecode(prog), \
        printf(message, str)); \
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
    printf(s.data);
    free(s.data);
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
What this function does is:
If there is an operation that pushes to stack and the operation that gets the value pushed by it is not before "end"
then a move_top instruction must be added before end so the top of the stack is still same as before "start"
returns numbers of operations added
*/
int keep_stack_top_unchanged(bytecode_program* prog, int start, int end){
    int instructions_added=0;
    // count how many objects are pushed to the stack and how many are taken from it
    int counter=0;
    for(int pointer=start; pointer<=end; pointer++){
        counter+=pushes_to_stack(prog->code[pointer].type);
        counter-=gets_from_stack(prog->code[pointer]);
    }
    if(counter>0){
        // add move_top instruction that moves the top object by counter
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
            if(prog->code[p].type!=b_no_op){
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
    LOG_CHANGE("Unoptimised bytecode:\n%s\n");
    for(int pointer=count_instructions(prog->code); pointer>=0; pointer--){
        if(prog->code[pointer].type==b_set && prog->code[pointer+1].type==b_discard
           && path_length(prog->code, pointer)<=2){// don't optimise nested paths like table.key, only single name paths
            if(LOG_BYTECODE_OPTIMISATIONS){
                printf("Found a set instruction\n");
                highlight_instructions(prog, '>', pointer-path_length(prog->code, pointer)+1, pointer);
            }
            bool first_get_removal=true;
            bool used=false;
            for(int search_pointer=pointer+2; prog->code[search_pointer].type!=b_end; search_pointer++){
                if(changes_flow(prog->code[search_pointer].type) || prog->code[search_pointer].type==b_get_scope){
                    // if flow changes we can't say if the variable is used later
                    // also if the scope ends with b_get_scope the scope is used as a table object later
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
                        printf("Removing discard instruction after set instruction:\n");
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
                printf("Removing set instruction:\n");
                fill_with_no_op(prog, pointer-path_length(prog->code, pointer)+1, pointer);
            }
        }
    }
    remove_no_ops(prog);
}

#undef LOG_BYTECODE_OPTIMISATIONS
#undef LOG_CHANGE