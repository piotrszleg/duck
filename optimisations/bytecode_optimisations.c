#include "bytecode_optimisations.h"

#define LOG_IF_ENABLED(message, ...) \
    if(print_optimisations){ \
        printf(message, ##__VA_ARGS__); \
    }

bool is_path_part(Instruction instr){
    return  instr.type==b_get || instr.type==b_table_get || instr.type==b_load_string;
}

int path_length(const Instruction* code,  int path_start){
    if(code[path_start].type!=b_get && code[path_start].type!=b_set){
        return 0;// path must start with either set or get
    }
    int p=1;
    for(; is_path_part(code[path_start-p]) && p<path_start+1; p++);
    return p;
}

/*int path_length(const Instruction* code,  int path_start){
    if(code[path_start].type!=b_get && code[path_start].type!=b_set){
        return 0;// path must start with either set or get
    }
    int balance=-1;
    if(code[path_start].type==b_set){
        balance--;// value assigned by set is not part of the path
    }
    for(int p=0; is_path_part(code[path_start-p]); p++){
        balance+=pushes_to_stack(code[p])-gets_from_stack(code[p]);
        if(balance==0){
            return p;
        }
    }
}*/

bool instructions_equal(Instruction a, Instruction b){
    return a.type==b.type && a.float_argument == b.float_argument;
}

bool paths_equal(const Instruction* code, int path1, int path2){
    int p=0;
    while(is_path_part(code[path1+p]) && is_path_part(code[path2+p])){
        if(!instructions_equal(code[path1+p], code[path2+p])){
            return false;
        }
        p--;
    }
    return true;
}

int count_instructions(Instruction* code){
    int p=0;
    for(; code[p].type!=b_end; p++);
    return p;
}

void move_instructions(BytecodeProgram* prog, int starting_index, int movement){
    int initial_count=count_instructions(prog->code);
    int moved_count=initial_count+movement;
    Instruction* new_code=malloc((moved_count+1)*sizeof(Instruction));
    InstructionInformation* new_information=malloc((moved_count+1)*sizeof(InstructionInformation));

    //if(print_optimisations)
    //    printf("move_instructions(%i, %i)\n", starting_index, movement);

    for(int i=0; i<moved_count; i++){
        if(i<=starting_index+movement){
            new_code[i]=prog->code[i];
            new_information[i]=prog->information[i];
        } else {
            new_code[i]=prog->code[i-movement];
            new_information[i]=prog->information[i-movement];
        }
    }
    Instruction end_instruction={b_end};
    new_code[moved_count]=end_instruction;
    free(prog->code);
    prog->code=new_code;
    free(prog->information);
    prog->information=new_information;
}

void highlight_instructions(BytecodeProgram* prog, char symbol, int start, int end){
    stream s;
    stream_init(&s, 64);

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

void insert_instruction(BytecodeProgram* prog, int point, Instruction instr, bool print_optimisations){
    move_instructions(prog, point-1, 1);
    prog->code[point]=instr;
    if(point>0){
        prog->information[point]=prog->information[point-1];
    } else {
        prog->information[point]=prog->information[point+1];
    }
    if(print_optimisations){
        highlight_instructions(prog, '+', point, point);
    }
}

void remove_instructions(BytecodeProgram* prog, int start, int end, bool print_optimisations){
    move_instructions(prog, end, start-end-1);// remove get path instructions
}

void fill_with_no_op(BytecodeProgram* prog, int start, int end, bool print_optimisations){
    if(print_optimisations){
        highlight_instructions(prog, '-', start, end);
    }
    Instruction instr={b_no_op};
    
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
int keep_stack_top_unchanged(BytecodeProgram* prog, int start, int end, bool print_optimisations){
    int instructions_added=0;
    // count how many objects are pushed to the stack and how many are taken from it
    int counter=0;
    for(int pointer=start; pointer<=end; pointer++){
        int stack_top_postion=counter;
        counter-=gets_from_stack(prog->code[pointer]);
        // if the Instruction takes more from the stack than was placed
        // before start, then we need to move the current stack top back,
        // because it certainly wants the values placed on stack before start point
        if(counter<0){
            while(counter<0){
                Instruction push_to_top_instruction={b_push_to_top};
                push_to_top_instruction.uint_argument=stack_top_postion-counter;
                insert_instruction(prog, pointer, push_to_top_instruction, print_optimisations);
                instructions_added++;
                // the Instruction was added after the pointer, so it needs to be skipped
                // also the end needs to be moved to compensate for that
                pointer++;
                end++;
                counter++;
            }
            // now the items that were on top of the stack are beneath the ones pushed to the top
            // in the while block, so they need to be pushed back to the top so that the order
            // is preserved
            for(int i=0; i<stack_top_postion; i++){
                Instruction push_to_top_instruction={b_push_to_top};
                push_to_top_instruction.uint_argument=stack_top_postion;
                insert_instruction(prog, pointer, push_to_top_instruction, print_optimisations);
                instructions_added++;
                pointer++;
                end++;
            }
        }
        counter+=pushes_to_stack(prog->code[pointer]);
    }
    if(counter>0){
        // add push_to_top Instruction that moves the object that was on top of the stack
        // before start point back to the top
        Instruction push_to_top_instruction={b_push_to_top};
        push_to_top_instruction.uint_argument=counter;
        insert_instruction(prog, end+1, push_to_top_instruction, print_optimisations);
        instructions_added++;
    }
    return instructions_added;
}

void remove_no_ops(BytecodeProgram* prog, bool print_optimisations){
    int block_start=0;
    bool inside_block=false;
    for(int p=0; prog->code[p].type!=b_end; p++){
        if(inside_block){
            if(prog->code[p].type!=b_no_op || prog->code[p+1].type==b_end){
                remove_instructions(prog, block_start, p-1, print_optimisations);
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

typedef enum {
    d_any,
    d_any_type,
    d_known_type,
    d_constant
} DummyType;

typedef struct
{
    unsigned int id;
    DummyType type;
    union {
        ObjectType known_type;
        Object constant_value;
    };
} Dummy;

typedef struct
{
    Dummy* inputs;
    Dummy* outputs;
} InstructionExpectations;

bool dummies_equal(Dummy* a, Dummy* b){
    if(a->type==d_any || b->type==d_any){
        return true;
    }
    return a->id==b->id;
}

static void print_id(unsigned int n){
    int interval='Z'-'A';
    do{
        printf("%c", 'A'+n%interval);
        n/=interval;
    } while(n);
}

void dummy_print(Dummy* dummy){
    switch(dummy->type){
        case d_any:
            printf("any");
            break;
        case d_any_type:
            print_id(dummy->id);
            break;
        case d_known_type:
            printf("(%s)", OBJECT_TYPE_NAMES[dummy->known_type]);
            print_id(dummy->id);
            break;
        case d_constant:
            print_id(dummy->id);
            USING_STRING(stringify_object(NULL, dummy->constant_value),
                printf("=%s", str));
            break;
        default:
            THROW_ERROR(BYTECODE_ERROR, "Incorrect dummy type %i.", dummy->type);
    }
}

static void print_expectations(Instruction* instructions, InstructionExpectations* expectations, int instructions_count){
    printf("Instructions expectations:\n");
    for(int p=0; p<instructions_count; p++){
        printf("%s (", INSTRUCTION_NAMES[instructions[p].type]);
        for(int i=0; i<gets_from_stack(instructions[p]); i++){
            if(i!=0){
                printf(", ");
            }
            dummy_print(&expectations[p].inputs[i]);
        }
        printf(")");
        printf("->(");
        for(int i=0; i<pushes_to_stack(instructions[p]); i++){
            if(i!=0){
                printf(", ");
            }
            dummy_print(&expectations[p].outputs[i]);
        }
        printf(")\n");
    }
}

void replace_dummy(Instruction* instructions, InstructionExpectations* expectations, int instructions_count, Dummy to_replace, Dummy replacement){
    for(int p=0; p<instructions_count; p++){
        for(int i=0; i<gets_from_stack(instructions[p]); i++){
            if(dummies_equal(&to_replace, &expectations[p].inputs[i])){
                expectations[p].inputs[i]=replacement;
            }
        }
        for(int i=0; i<pushes_to_stack(instructions[p]); i++){
            if(dummies_equal(&to_replace, &expectations[p].outputs[i])){
                expectations[p].outputs[i]=replacement;
            }
        }
    }
}

void remove_no_ops_stack(stack* instructions, stack* informations, stack* expectations){
    #define INSTRUCTION(nth) ((Instruction*)stack_index(instructions, (nth)))
    int block_start=0;
    bool inside_block=false;
    for(int p=0; p<stack_count(instructions); p++){
        if(inside_block){
            if(INSTRUCTION(p)->type!=b_no_op || INSTRUCTION(p)->type==b_end){
                stack_delete_range(instructions, block_start, p-1);
                stack_delete_range(informations, block_start, p-1);
                stack_delete_range(expectations, block_start, p-1);
                // move the pointer back by the number of instructions removed
                p=p-1-block_start;
                inside_block=false;
            }
        } 
        if(INSTRUCTION(p)->type==b_no_op){
            block_start=p;
            inside_block=true;
        }
    }
    #undef INSTRUCTION
}

void optimise_bytecode_with_stack(BytecodeProgram* prog, bool print_optimisations){
    for(int i=0; i<prog->sub_programs_count; i++){
        optimise_bytecode_with_stack(&prog->sub_programs[i], print_optimisations);
    }
    if(print_optimisations){
        USING_STRING(stringify_bytecode(prog),
            printf("Unoptimised bytecode:\n%s\n", str));
    }
    int instructions_count=count_instructions(prog->code)+1;
    stack provided, dummy_stack, expectations;
    stack_init(&provided, sizeof(Dummy), 64);
    stack_init(&dummy_stack, sizeof(Dummy), 128);
    stack_init(&expectations, sizeof(InstructionExpectations), instructions_count);
    unsigned int dummy_objects_counter=0;
    for(int p=0; p<instructions_count; p++){
        InstructionExpectations instr;
        int inputs_count=gets_from_stack(prog->code[p]);
        instr.inputs=malloc(sizeof(Dummy)*inputs_count);
        for(int i=0; i<inputs_count; i++){
            if(!stack_empty(&dummy_stack)){
                instr.inputs[i]=*(Dummy*)stack_pop(&dummy_stack);
            } else {
                Dummy new_dummy={dummy_objects_counter};
                new_dummy.type=d_any_type;
                dummy_objects_counter++;
                stack_push(&provided, &new_dummy);
                instr.inputs[i]=new_dummy;
            }
        }
        int outputs_count=pushes_to_stack(prog->code[p]);
        instr.outputs=malloc(sizeof(Dummy)*outputs_count);
        for(int i=0; i<outputs_count; i++){
            Dummy new_dummy={dummy_objects_counter};
            new_dummy.type=d_any_type;
            dummy_objects_counter++;
            instr.outputs[i]=new_dummy;
            stack_push(&dummy_stack, &new_dummy);
        }
        stack_push(&expectations, &instr);
    }

    print_expectations(prog->code, stack_get_data(&expectations), instructions_count);

    stack instructions, informations;
    stack_from(&instructions, sizeof(Instruction), prog->code, instructions_count);
    stack_from(&informations, sizeof(InstructionInformation), prog->information, instructions_count);

    #define FILL_WITH_NO_OP(start, end) \
        for(int i=start; i<=end; i++) { \
            ((Instruction*)stack_index(&instructions, i))->type=b_no_op; \
            /* InstructionExpectations* exp=(InstructionExpectations*)stack_index(&instructions, i); */ \
            /* free(exp->inputs); */ \
            /* exp->inputs=NULL; */ \
            /* free(exp->outputs); */ \
            /* exp->outputs=NULL; */ \
        }
    
    #define INSTRUCTION(nth) ((Instruction*)stack_index(&instructions, (nth)))
    #define EXPECTATION(nth) ((InstructionExpectations*)stack_index(&expectations, (nth)))
    #define CODE ((Instruction*)stack_get_data(&instructions))
    for(int pointer=instructions_count-1; pointer>=0; pointer--){
        if(INSTRUCTION(pointer)->type==b_call){
            if(INSTRUCTION(pointer+1)->type==b_return || INSTRUCTION(pointer+1)->type==b_end){
                INSTRUCTION(pointer)->type=b_tail_call;
            }
        } else if(prog->code[pointer].type==b_set && prog->code[pointer+1].type==b_discard
           && !prog->code[pointer].bool_argument /* argument tells whether the variable is used in closure, we can't tell if the closure changes the variable*/
           && path_length(prog->code, pointer)<=2){// don't optimise nested paths like Table.key, only single name paths
            if(print_optimisations){
                printf("Found a set Instruction\n");
                prog->code=stack_get_data(&instructions);
                highlight_instructions(prog, '>', pointer-path_length(prog->code, pointer)+1, pointer);
            }
            bool first_get_removal=true;
            bool used=false;
            for(int search_pointer=pointer+2; INSTRUCTION(search_pointer)->type!=b_end; search_pointer++){
                if(changes_flow(INSTRUCTION(search_pointer)->type)){
                    // we can't tell if the variable is used later
                    used=true;
                    break;
                }
                if(changes_scope(INSTRUCTION(search_pointer)->type)){
                    // we optimised all gets in this scope so the variable isn't needed anymore
                    break;
                }
                if(INSTRUCTION(search_pointer)->type==b_get && paths_equal(CODE, pointer-1, search_pointer-1)){
                    if(print_optimisations){
                        printf("Found a corresponding get Instruction\n");
                        prog->code=stack_get_data(&instructions);
                        highlight_instructions(prog, '>', search_pointer-path_length(CODE, pointer)+1, search_pointer);
                    }
                    // for now only single name variables are optimised
                    int get_path_length=2;//path_length(CODE, search_pointer);

                    if(first_get_removal){
                        LOG_IF_ENABLED("Removing discard Instruction after set Instruction:\n");
                        FILL_WITH_NO_OP(pointer+1, pointer+1);
                        first_get_removal=false;
                    } else{
                        Instruction double_instruction={b_double};
                        stack_insert(&instructions, pointer+1, &double_instruction);
                        InstructionExpectations expect;
                        expect.inputs=malloc(sizeof(InstructionExpectations));
                        expect.inputs[0].type=d_any_type;
                        expect.inputs[0].id=((InstructionExpectations*)stack_index(&expectations, pointer))->outputs[0].id;
                        expect.outputs=malloc(sizeof(InstructionExpectations)*2);
                        expect.outputs[0].type=d_any_type;
                        expect.outputs[0].id=((InstructionExpectations*)stack_index(&expectations, pointer))->outputs[0].id;
                        expect.outputs[1].type=d_any_type;
                        expect.outputs[1].id=((InstructionExpectations*)stack_index(&expectations, pointer))->outputs[0].id;
                        stack_insert(&expectations, pointer+1, &expect);
                        stack_insert(&informations, pointer+1, stack_index(&informations, pointer));
                        search_pointer++;
                    }
                    // search for references to dummy object and replace them with the one
                    replace_dummy((Instruction*)stack_get_data(&instructions), (InstructionExpectations*)stack_get_data(&expectations), stack_count(&instructions),
                    EXPECTATION(search_pointer)->outputs[0], 
                    EXPECTATION(pointer)->outputs[0]);
                    //search_pointer+=keep_stack_top_unchanged(prog, pointer+1, search_pointer-get_path_length, print_optimisations);
                    FILL_WITH_NO_OP(search_pointer-get_path_length+1, search_pointer);

                    if(search_pointer>=stack_count(&instructions)){
                        break;// after removing the path search_pointer points to b_end, if loop continued from this point it would get past the code's end
                    }
                }
            }
            if(!used){
                // the variable isn't used in it's own scope and in any closure, so it can be removed
                LOG_IF_ENABLED("Removing set Instruction:\n");
                replace_dummy((Instruction*)stack_get_data(&instructions), (InstructionExpectations*)stack_get_data(&expectations), stack_count(&instructions),
                EXPECTATION(pointer)->outputs[0], 
                EXPECTATION(pointer)->inputs[1]);
                FILL_WITH_NO_OP(pointer-path_length(CODE, pointer)+1, pointer);
            }
        }
    }
    while(!stack_empty(&dummy_stack)){
        stack_pop(&dummy_stack);
    }
    #define STACK(nth) ((Dummy*)stack_index(&dummy_stack, stack_count(&dummy_stack)-1-(nth)))
    for(int p=0; p<stack_count(&instructions); p++){
        for(int i=0; i<gets_from_stack(*INSTRUCTION(p)); i++){
            Dummy* top=(Dummy*)stack_index(&dummy_stack, stack_count(&dummy_stack)-1-i);
            Dummy* expected=&EXPECTATION(p)->inputs[i];
            if(!dummies_equal(top, expected)){
                int stack_depth=stack_count(&dummy_stack);
                for(int j=1; j<stack_depth; j++){
                    if(dummies_equal(STACK(j), expected)){
                        Instruction swap_instruction={b_swap};
                        swap_instruction.swap_argument.left=j;
                        swap_instruction.swap_argument.right=i;
                        InstructionExpectations push_expect;
                        int affected=j+1;
                        push_expect.inputs=malloc(sizeof(Dummy)*affected);
                        push_expect.outputs=malloc(sizeof(Dummy)*affected);
                        Dummy any;
                        any.type=d_any;
                        for(int k=0; k<affected; k++){    
                            push_expect.inputs[k]=push_expect.outputs[k]=any;
                        }
                        push_expect.inputs[swap_instruction.swap_argument.left]=*STACK(swap_instruction.swap_argument.left);
                        push_expect.inputs[swap_instruction.swap_argument.right]=*STACK(swap_instruction.swap_argument.right);
                        push_expect.outputs[swap_instruction.swap_argument.left]=*STACK(swap_instruction.swap_argument.right);
                        push_expect.outputs[swap_instruction.swap_argument.right]=*STACK(swap_instruction.swap_argument.left);
                        Dummy temp=*STACK(swap_instruction.swap_argument.left);
                        *STACK(swap_instruction.swap_argument.left)=*STACK(swap_instruction.swap_argument.right);
                        *STACK(swap_instruction.swap_argument.right)=temp;
                        stack_insert(&instructions, p-1, &swap_instruction);
                        stack_insert(&expectations, p-1, &push_expect);
                        stack_insert(&informations, p-1, stack_index(&informations, p));
                        p++;
                        instructions_count++;
                    }
                }
            }
        }
        for(int j=0; j<gets_from_stack(*INSTRUCTION(p)); j++){
            stack_pop(&dummy_stack);
        }
        for(int i=0; i<pushes_to_stack(*INSTRUCTION(p)); i++){
            stack_push(&dummy_stack, &EXPECTATION(p)->outputs[i]);
        }
    }
    printf("\nbfeor removing noop");
    print_expectations(stack_get_data(&instructions), stack_get_data(&expectations), stack_count(&instructions));
    remove_no_ops_stack(&instructions, &informations, &expectations);
    prog->code=stack_get_data(&instructions);
    prog->information=stack_get_data(&informations);

    printf("\n");
    print_expectations(prog->code, stack_get_data(&expectations), stack_count(&instructions));

    stack_deinit(&provided);
    stack_deinit(&dummy_stack);
    stack_deinit(&expectations);

    #undef FILL_WITH_NO_OP
    #undef INSTRUCTION
    #undef EXPECTATION
    #undef CODE
}

void optimise_bytecode(BytecodeProgram* prog, bool print_optimisations){
    for(int i=0; i<prog->sub_programs_count; i++){
        optimise_bytecode(&prog->sub_programs[i], print_optimisations);
    }
    if(print_optimisations){
        USING_STRING(stringify_bytecode(prog),
            printf("Unoptimised bytecode:\n%s\n", str));
    }
    for(int pointer=count_instructions(prog->code)-1; pointer>=0; pointer--){
        if(prog->code[pointer].type==b_call){
            if(prog->code[pointer+1].type==b_return || prog->code[pointer+1].type==b_end){
                prog->code[pointer].type=b_tail_call;
            }
        } else if(prog->code[pointer].type==b_set && prog->code[pointer+1].type==b_discard
           && !prog->code[pointer].bool_argument /* argument tells whether the variable is used in closure, we can't tell if the closure changes the variable*/
           && path_length(prog->code, pointer)<=2){// don't optimise nested paths like Table.key, only single name paths
            if(print_optimisations){
                printf("Found a set Instruction\n");
                highlight_instructions(prog, '>', pointer-path_length(prog->code, pointer)+1, pointer);
            }
            bool first_get_removal=true;
            bool used=false;
            for(int search_pointer=pointer+2; prog->code[search_pointer].type!=b_end; search_pointer++){
                if(changes_flow(prog->code[search_pointer].type)){
                    // we can't tell if the variable is used later
                    used=true;
                    break;
                }
                if(changes_scope(prog->code[search_pointer].type)){
                    // we optimised all gets in this scope so the variable isn't needed anymore
                    break;
                }
                if(prog->code[search_pointer].type==b_get && paths_equal(prog->code, pointer-1, search_pointer-1)){
                    if(print_optimisations){
                        printf("Found a corresponding get Instruction\n");
                        highlight_instructions(prog, '>', search_pointer-path_length(prog->code, pointer)+1, search_pointer);
                    }
                    // for now only single name variables are optimised
                    int get_path_length=2;//path_length(prog->code, search_pointer);

                    if(first_get_removal){
                        LOG_IF_ENABLED("Removing discard Instruction after set Instruction:\n");
                        fill_with_no_op(prog, pointer+1, pointer+1, print_optimisations);
                        first_get_removal=false;
                    } else{
                        Instruction double_instruction={b_double};
                        insert_instruction(prog, pointer+1, double_instruction, print_optimisations);
                        search_pointer++;
                    }
                    search_pointer+=keep_stack_top_unchanged(prog, pointer+1, search_pointer-get_path_length, print_optimisations);
                    fill_with_no_op(prog, search_pointer-get_path_length+1, search_pointer, print_optimisations);

                    if(search_pointer>=count_instructions(prog->code)){
                        break;// after removing the path search_pointer points to b_end, if loop continued from this point it would get past the code's end
                    }
                }
            }
            if(!used){
                // the variable isn't used in it's own scope and in any closure, so it can be removed
                LOG_IF_ENABLED("Removing set Instruction:\n");
                fill_with_no_op(prog, pointer-path_length(prog->code, pointer)+1, pointer, print_optimisations);
            }
        }
    }
    remove_no_ops(prog, print_optimisations);
}

#undef LOG