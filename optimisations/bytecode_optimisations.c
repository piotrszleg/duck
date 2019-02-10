#include "bytecode_optimisations.h"

#define LOG_BYTECODE_OPTIMISATIONS 1
#define LOG_CHANGE(message) \
    if(LOG_BYTECODE_OPTIMISATIONS){ \
        USING_STRING(stringify_bytecode(prog), \
        printf(message, str)); \
    }

bool is_path_part(instruction instr){
    return instr.type==b_get || instr.type==b_load_string;
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

    if(LOG_BYTECODE_OPTIMISATIONS)
        printf("move_instructions(%i, %i)\n", starting_index, movement);

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

/*
What this function does is:
If there is an operation that pushes to stack and the operation that gets the value pushed by it is not before "end"
then a swap instruction must be added before end so the top of the stack is still same as before "start"
returns true if swap operation was added
*/
bool keep_stack_top_unchanged(bytecode_program* prog, int start, int end){
    // count how many objects are pushed to the stack and how many are taken from it
    int counter=0;
    for(int pointer=start; pointer<end; pointer++){
        counter+=pushes_to_stack(prog->code[pointer].type);
        counter-=gets_from_stack(prog->code[pointer]);
    }
    if(counter>0){
        // add swap instruction that swaps "counter" amount of items
        move_instructions(prog, end-1, 1);
        instruction swap_instruction={b_swap, counter};
        prog->code[end]=swap_instruction;
        LOG_CHANGE("added swap:\n%s\n")
        return true;
    } else{
        return false;
    }
}

void optimise_bytecode(bytecode_program* prog){
    LOG_CHANGE("Unoptimised bytecode:\n%s\n");
    for(int pointer=count_instructions(prog->code); pointer>=0; pointer--){
        if(prog->code[pointer].type==b_set && prog->code[pointer+1].type==b_discard
           && path_length(prog->code, pointer)<=2){// don't optimise nested paths like table.key, only single name paths
            if(LOG_BYTECODE_OPTIMISATIONS){
                printf("I found a set instruction: %i, path length is: %i\n", pointer, path_length(prog->code, pointer));
            }
            bool first_get_removal=true;
            bool used=false;
            for(int search_pointer=pointer+2; prog->code[search_pointer].type!=b_end; search_pointer++){
                if(changes_flow(prog->code[search_pointer].type) || prog->code[search_pointer].type==b_get_scope){
                    // if flow changes we can't say if the variable is used later
                    // also if the scope ends with b_get_scope the scope is used as an table object later
                    used=true;
                    break;
                }
                if(changes_scope(prog->code[search_pointer].type)){
                    // we optimised all gets in this scope so the varaible isn't needed anymore
                    break;
                }
                if(prog->code[search_pointer].type==b_get && paths_equal(prog->code, pointer-1, search_pointer-1)){
                    // for now only single name variables are optimised
                    int get_path_length=2;//path_length(prog->code, search_pointer);

                    if(keep_stack_top_unchanged(prog, pointer+2, search_pointer+1-get_path_length)){
                        search_pointer++;// if swap instruction was added search needs to be moved by one to point at the correct instruction
                    }
                    move_instructions(prog, search_pointer, -get_path_length);// remove get path instructions
                    if(first_get_removal){
                        move_instructions(prog, pointer+1, -1);// remove discard instruction
                        LOG_CHANGE("removed get and discard instructions:\n%s\n");
                        first_get_removal=false;
                    } else{
                        move_instructions(prog, pointer, 1);
                        instruction double_instruction={b_double, 0};
                        prog->code[pointer+1]=double_instruction;
                        LOG_CHANGE("removed get and added double instruction:\n%s\n")
                    }
                }
            }
            if(!used){
                move_instructions(prog, pointer, -path_length(prog->code, pointer));// remove set path instructions
            }
        }
    }
    LOG_CHANGE("Optimised bytecode:\n%s\n");
}

#undef LOG_BYTECODE_OPTIMISATIONS
#undef LOG_CHANGE