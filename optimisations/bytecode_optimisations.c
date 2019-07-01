#include "bytecode_optimisations.h"

#define LOG_IF_ENABLED(message, ...) \
    if(print_optimisations){ \
        printf(message, ##__VA_ARGS__); \
    }

bool is_path_part(Instruction instr){
    return  instr.type==b_get || instr.type==b_table_get || instr.type==b_load_string || instr.type==b_load_int || instr.type==b_load_float;
}

int path_length(const Instruction* code,  int path_start){
    if(code[path_start].type==b_get || code[path_start].type==b_set || code[path_start].type==b_table_get){
        int p=1;
        for(; is_path_part(code[path_start-p]) && p<path_start+1; p++);
        return p;
    } else {
        return 0;
    }
}

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

void highlight_instructions(BytecodeProgram* prog, char symbol, int start, int end){

    int pointer=0;
    while(prog->code[pointer].type!=b_end){
        if(pointer>=start && pointer<=end){
            printf("%c ", symbol);
        } else {
            printf("  ");
        }
        char stringified_instruction[64];
        stringify_instruction(prog, (char*)&stringified_instruction, prog->code[pointer], 64);
        printf(stringified_instruction);
        printf("\n");
        pointer++;
    }
}

typedef enum {
    d_any,
    d_any_type,
    d_known_type,
    d_constant,
    d_or
} DummyType;

typedef struct Dummy Dummy;
struct Dummy {
    gc_Pointer gcp;
    unsigned int id;
    DummyType type;
    union {
        ObjectType known_type;
        Object constant_value;
        struct {
            Dummy* left;
            Dummy* right;
        } or;
    };
};

void dummy_mark_children(Dummy* dummy){
    dummy->gcp.gco.marked=true;
    if(dummy->type==d_or){
        dummy_mark_children(dummy->or.left);
        dummy_mark_children(dummy->or.right);
    }
}

void dummy_dereference_children(Executor* E, Dummy* dummy){
    dummy->gcp.gco.marked=true;
    if(dummy->type==d_or){
        gc_object_dereference(E, (gc_Object*)dummy->or.left);
        gc_object_dereference(E, (gc_Object*)dummy->or.right);
    }
}

void dummy_free(Dummy* dummy){
    if(dummy->type==d_or){
        dummy_free(dummy->or.left);
        dummy_free(dummy->or.right);
    }
    free(dummy);
}

Dummy* new_dummy(Executor* E){
    Dummy* result=malloc(sizeof(Dummy));
    gc_pointer_init(E, (gc_Pointer*)result, (gc_PointerFreeFunction)dummy_free);
    result->gcp.mark_children=(gc_PointerMarkChildrenFunction)dummy_mark_children;
    result->gcp.dereference_children=(gc_PointerDereferenceChildrenFunction)dummy_dereference_children;
    return result;
}

typedef struct
{
    bool visited;
    Dummy** inputs;
    int inputs_count;
    Dummy** outputs;
    int outputs_count;
} Transformation;

bool dummy_is_typed(const Dummy* dummy){
    return dummy->type==d_known_type || dummy->type==d_constant;
}

ObjectType dummy_type(const Dummy* dummy){
    if(dummy->type==d_known_type){
        return dummy->known_type;
    } else if(dummy->type==d_constant){
        return dummy->constant_value.type;
    } else {
        return t_null;
    }
}

bool has_side_effects(Instruction* instruction, Transformation* transformation) {
    switch(instruction->type) {
        case b_null:
        case b_load_int:
        case b_load_float:
        case b_load_string:
        case b_table_literal:
        case b_pre_function:
        case b_function:
        case b_double:
            return false;
        case b_prefix:
        case b_binary:
            // only operations on tables and functions can cause side effects
            return dummy_is_typed(transformation->inputs[0])
            &&     dummy_is_typed(transformation->inputs[1])
            &&     dummy_type(transformation->inputs[0])!=t_table
            &&     dummy_type(transformation->inputs[1])!=t_table
            &&     dummy_type(transformation->inputs[0])!=t_function
            &&     dummy_type(transformation->inputs[1])!=t_function;
        default: return true;
    }
}

// function writes to transformation outputs the result of evaluating instruction with it's input
void predict_instruction_output(Executor* E, Instruction* instr, char* constants, unsigned int* id_counter, Transformation* transformation){
    Dummy** outputs=transformation->outputs;
    Dummy** inputs=transformation->inputs;
    if(carries_stack(instr->type)){
        int i=0;
        // jump_not takes one item from the stack as a predicate
        // so it needs to be skipped
        if(instr->type==b_jump_not){
            i++;
        }
        for(; i<transformation->inputs_count; i++){
            outputs[transformation->inputs_count-1-i]=inputs[i];
        }
        return;
    }
    switch (instr->type){
        case b_null:
            outputs[0]=new_dummy(E);
            outputs[0]->id=(*id_counter)++;
            outputs[0]->type=d_constant;
            outputs[0]->constant_value=null_const;
            break;
        case b_load_int:
            outputs[0]=new_dummy(E);
            outputs[0]->id=(*id_counter)++;
            outputs[0]->type=d_constant;
            outputs[0]->constant_value=to_int(instr->int_argument);
            break;
        case b_load_float:
            outputs[0]=new_dummy(E);
            outputs[0]->id=(*id_counter)++;
            outputs[0]->type=d_constant;
            outputs[0]->constant_value=to_float(instr->float_argument);
            break;
        case b_load_string:
            outputs[0]=new_dummy(E);
            outputs[0]->id=(*id_counter)++;
            outputs[0]->type=d_constant;
            outputs[0]->constant_value=to_string(constants+instr->uint_argument);
            break;
        case b_table_literal:
            outputs[0]=new_dummy(E);
            outputs[0]->id=(*id_counter)++;
            outputs[0]->type=d_known_type;
            outputs[0]->known_type=t_table;
            break;
        case b_pre_function:
            outputs[0]=new_dummy(E);
            outputs[0]->id=(*id_counter)++;
            outputs[0]->type=d_known_type;
            outputs[0]->known_type=t_function;
            break;
        case b_function:
            outputs[0]=inputs[0];
            break;
        case b_double:
            outputs[0]=inputs[0];
            outputs[1]=inputs[0];
            break;
        case b_push_to_top:
            outputs[instr->uint_argument]=inputs[0];
            outputs[0]=inputs[instr->uint_argument];
            for(int i=1; i<instr->uint_argument-1; i++){
                outputs[i]=outputs[instr->uint_argument-2-i];
            }
            break;
        case b_swap:
        {
            for(int i=0; i<transformation->outputs_count; i++){
                outputs[transformation->outputs_count-1-i]=inputs[i];
            }
            int left=transformation->outputs_count-1-instr->swap_argument.left;
            int right=transformation->outputs_count-1-instr->swap_argument.right;
            Dummy* temp=outputs[left];
            outputs[left]=outputs[right];
            outputs[right]=temp;
            break;
        }
        case b_binary:
        {
            if((inputs[1]->type==d_known_type||inputs[1]->type==d_constant)
            && (inputs[2]->type==d_known_type||inputs[2]->type==d_constant)
            && dummy_type(inputs[1])==dummy_type(inputs[2])){
                outputs[0]=new_dummy(E);
                outputs[0]->id=(*id_counter)++;
                outputs[0]->type=d_known_type;
                outputs[0]->known_type=dummy_type(inputs[1]);
                break;
            }
            // intentional fall through
        }
        default:
        {
            for(int i=0; i<transformation->outputs_count; i++){
                outputs[i]=new_dummy(E);
                outputs[i]->id=(*id_counter)++;
                outputs[i]->type=d_any_type;
            }
        }
    }
}

bool dummies_equal(const Dummy* a, const Dummy* b){
    if(a->type==d_any || b->type==d_any){
        return true;
    } else {
        return a->id==b->id;
    }
}

bool dummies_compatible(const Dummy* a, const Dummy* b){
    if(a->type==d_any || b->type==d_any){
        return true;
    } else if(a->type==d_or){
        return dummies_compatible(a->or.left, b) || dummies_compatible(a->or.right, b);
    } else if(b->type==d_or){
        return dummies_compatible(a, b->or.left) || dummies_compatible(a, b->or.right);
    } else {
        return a->id==b->id;
    }
}

bool dummy_contains(const Dummy* a, const Dummy* b){
    if(a->type==d_any || b->type==d_any){
        return true;
    } else if(a->type==d_or){
        return dummy_contains(a->or.left, b) || dummy_contains(a->or.right, b);
    } else if(b->type==d_or){
        return dummy_contains(a, b->or.left) || dummy_contains(a, b->or.right);
    } else {
        return a->id==b->id;
    }
}

static void print_id(unsigned int n){
    int interval='Z'-'A';
    do{
        printf("%c", 'A'+n%interval);
        n/=interval;
    } while(n);
}

void dummy_print(const Dummy* dummy){
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
            USING_STRING(stringify_object(NULL, dummy->constant_value),
                printf("%s", str));
            break;
        case d_or:
            printf("(");
            dummy_print(dummy->or.left);
            printf(" or ");
            dummy_print(dummy->or.right);
            printf(")");
            break;
        default:
            THROW_ERROR(BYTECODE_ERROR, "Incorrect dummy type %i.", dummy->type);
    }
    //printf("<%i>", dummy->gcp.gco.ref_count);
}

static void print_transformations(Instruction* instructions, Transformation* transformations, int instructions_count){
    printf("Instructions transformations:\n");
    for(int p=0; p<instructions_count; p++){
        printf("%s (", INSTRUCTION_NAMES[instructions[p].type]);
        for(int i=0; i<transformations[p].inputs_count; i++){
            if(i!=0){
                printf(", ");
            }
            dummy_print(transformations[p].inputs[i]);
        }
        printf(")");
        printf("->(");
        for(int i=0; i<transformations[p].outputs_count; i++){
            if(i!=0){
                printf(", ");
            }
            dummy_print(transformations[p].outputs[i]);
        }
        printf(")\n");
    }
}

void dummy_replace(Executor* E, Dummy** dummy, Dummy* to_replace, Dummy* replacement){
    if((*dummy)->type==d_or){
        dummy_replace(E, &(*dummy)->or.left, to_replace, replacement);
        dummy_replace(E, &(*dummy)->or.right, to_replace, replacement);
    } else if(dummies_equal(*dummy, to_replace)) {
        gc_object_dereference(E, (gc_Object*)*dummy);
        gc_object_reference((gc_Object*)replacement);
        *dummy=replacement;
    }
}

void replace_dummies_in_transformations(Executor* E, Instruction* instructions, Transformation* transformations, int instructions_count, Dummy* to_replace, Dummy* replacement){
    for(int p=0; p<instructions_count; p++){
        for(int i=0; i<transformations[p].inputs_count; i++){
            dummy_replace(E, &transformations[p].inputs[i], to_replace, replacement);
        }
        for(int i=0; i<transformations[p].outputs_count; i++){
            dummy_replace(E, &transformations[p].outputs[i], to_replace, replacement);
        }
    }
}

void transformation_init(Transformation* transformation, int inputs_count, int outputs_count){
    transformation->inputs_count=inputs_count;
    transformation->inputs=malloc(sizeof(Dummy*)*transformation->inputs_count);
    if(transformation->inputs_count>0){
        CHECK_ALLOCATION(transformation->inputs)
    }
    transformation->outputs_count=outputs_count;
    transformation->outputs=malloc(sizeof(Dummy*)*transformation->outputs_count);
    if(transformation->outputs_count>0){
        CHECK_ALLOCATION(transformation->outputs)
    }
}

void transformation_from_instruction(Transformation* transformation, Instruction* instruction){
    transformation_init(transformation, gets_from_stack(*instruction), pushes_to_stack(*instruction));
}

void transformation_deinit(Executor* E, Transformation* transformation){
    for(int i=0; i<transformation->inputs_count; i++){
        gc_object_dereference(E, (gc_Object*)transformation->inputs[i]);
    }
    transformation->inputs_count=0;
    for(int i=0; i<transformation->outputs_count; i++){
        gc_object_dereference(E, (gc_Object*)transformation->outputs[i]);
    }
    transformation->outputs_count=0;
    free(transformation->inputs);
    transformation->inputs=NULL;
    free(transformation->outputs);
    transformation->outputs=NULL;
}

void remove_no_ops(Executor* E, vector* instructions, vector* informations, vector* transformations){
    #define INSTRUCTION(nth) ((Instruction*)vector_index(instructions, (nth)))
    int block_start=0;
    bool inside_block=false;
    for(int p=0; p<vector_count(instructions); p++){
        if(inside_block){
            if(INSTRUCTION(p)->type!=b_no_op || INSTRUCTION(p)->type==b_end){
                for(int i=block_start; i<=p-1; i++){
                    transformation_deinit(E, (Transformation*)vector_index(transformations, i));
                }
                vector_delete_range(instructions, block_start, p-1);
                vector_delete_range(informations, block_start, p-1);
                vector_delete_range(transformations, block_start, p-1);
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

typedef struct {
    vector branches;
    bool started;
    bool revisit;
    int last;
}BytecodeProgressState;

int find_label(Instruction* code, int index){
    for(int search_pointer=0; code[search_pointer].type!=b_end; search_pointer++){
        if(code[search_pointer].type==b_label && code[search_pointer].uint_argument==index){
            return search_pointer;
        }
    }
    return -1;
}

int count_branches(Instruction* code){
    int counter=0;
    for(int p=0; code[p].type!=b_end; p++){
        if(code[p].type==b_jump_not){
            counter++;
        }
    }
    return counter;
}

int bytecode_progress_start(Instruction* code, BytecodeProgressState* state){
    state->started=false;
    state->last=0;
    vector_init(&state->branches, sizeof(int), 8);
    state->revisit=false;
    return 0;
}

int bytecode_progress_next(Instruction* code, BytecodeProgressState* state){
    vector* branches=&state->branches;
    int index=state->last;
    if(finishes_program(code[index].type)){
        if(state->revisit){
            state->revisit=false;
            index=0;
            state->last=index;
            return index;
        } else {
            vector_deinit(branches);
            return -1;
        }
    }
    if(code[index].type==b_jump_not){
        if(vector_search(branches, &index)>0){
            index=find_label(code, code[index].uint_argument);
        } else {
            vector_push(branches, &index);
            state->revisit=true;
            index++;
        }
    } else if(code[index].type==b_jump){
        index=find_label(code, code[index].uint_argument);
    } else {
        index++;
    }
    state->last=index;
    return index;
}

void optimise_bytecode(Executor* E, BytecodeProgram* prog, bool print_optimisations){
    for(int i=0; i<prog->sub_programs_count; i++){
        optimise_bytecode(E, &prog->sub_programs[i], print_optimisations);
    }
    if(print_optimisations){
        USING_STRING(stringify_bytecode(prog),
            printf("Unoptimised bytecode:\n%s\n", str));
    }

    // step 1: deduce how the objects will flow from one instruction to the next
    // also get a list of externally provided objects

    int instructions_count=count_instructions(prog->code)+1;
    vector provided, stack, transformations, instructions, informations;
    vector_init(&provided, sizeof(Dummy*), 64);
    vector_init(&stack, sizeof(Dummy*), 128);
    vector_init(&transformations, sizeof(Transformation), instructions_count);
    vector_extend(&transformations, instructions_count);
    vector_from(&instructions, sizeof(Instruction), prog->code, instructions_count);
    vector_from(&informations, sizeof(InstructionInformation), prog->information, instructions_count);
    #define INSTRUCTION(nth) ((Instruction*)vector_index(&instructions, (nth)))
    #define TRANSFORMATION(nth) ((Transformation*)vector_index(&transformations, (nth)))
    #define CODE ((Instruction*)vector_get_data(&instructions))
    #define FILL_WITH_NO_OP(start, end) \
        prog->code=(Instruction*)vector_get_data(&instructions); \
        if(print_optimisations){ \
            highlight_instructions(prog, '-', start, end); \
        } \
        for(int i=start; i<=end; i++) { \
            transformation_deinit(E, TRANSFORMATION(i)); \
            INSTRUCTION(i)->type=b_no_op; \
        }
    unsigned int dummy_objects_counter=0;

    BytecodeProgressState progress_state;
    for(int p=bytecode_progress_start(prog->code, &progress_state); p!=-1; p=bytecode_progress_next(prog->code, &progress_state)){
        if(p==0){
            for(int i=vector_count(&provided)-1; i>=0; i--){
                vector_push(&stack, vector_index(&provided, i));
            }
        }
        if(!TRANSFORMATION(p)->visited){
            Transformation transformation;
            if(carries_stack(prog->code[p].type)){
                // jump_not takes one item from the stack as a predicate
                if(prog->code[p].type==b_jump_not){
                    transformation_init(&transformation, vector_count(&stack), vector_count(&stack)-1);
                } else {
                    transformation_init(&transformation, vector_count(&stack), vector_count(&stack));
                }
            } else {
                transformation_from_instruction(&transformation, &prog->code[p]);
            }
            
            for(int i=0; i<transformation.inputs_count; i++){
                if(!vector_empty(&stack)){
                    transformation.inputs[i]=*(Dummy**)vector_pop(&stack);
                } else {
                    Dummy* dummy=new_dummy(E);
                    dummy->id=dummy_objects_counter++;
                    dummy->type=d_any_type;
                    vector_push(&provided, &dummy);
                    transformation.inputs[i]=dummy;
                    // in this case dummy is also refrenced by provided
                    gc_object_reference((gc_Object*)transformation.inputs[i]);
                }
                // input is referenced by the transformation
                gc_object_reference((gc_Object*)transformation.inputs[i]);
            }
            predict_instruction_output(E, &prog->code[p], (char*)prog->constants, &dummy_objects_counter, &transformation);
            for(int i=0; i<transformation.outputs_count; i++){
                vector_push(&stack, &transformation.outputs[i]);
                // output is referenced by the transformation
                gc_object_reference((gc_Object*)transformation.outputs[i]);
            }
            *TRANSFORMATION(p)=transformation;
            TRANSFORMATION(p)->visited=true;
        } else {
            bool inputs_changed=false;
            for(int i=0; i<TRANSFORMATION(p)->inputs_count; i++){
                Dummy* from_stack=*(Dummy**)vector_pop(&stack);
                Dummy* expected=(Dummy*)TRANSFORMATION(p)->inputs[i];
                if(!dummies_equal(expected, from_stack)){
                    if(dummy_contains(from_stack, expected)){
                        gc_object_dereference(E, (gc_Object*)expected);
                        gc_object_reference((gc_Object*)from_stack);
                        TRANSFORMATION(p)->inputs[i]=from_stack;
                        inputs_changed=true;
                    } else if(!dummy_contains(expected, from_stack)){
                        Dummy* or_dummy=new_dummy(E);
                        or_dummy->type=d_or;
                        or_dummy->or.left=expected;
                        or_dummy->or.right=from_stack;
                        // expected is already referenced by the instruction
                        gc_object_reference((gc_Object*)or_dummy);// or_dummy referenced by transformation
                        gc_object_reference((gc_Object*)from_stack);// referenced by or_dummy
                        TRANSFORMATION(p)->inputs[i]=or_dummy;
                        inputs_changed=true;
                    }
                }
            }
            if(inputs_changed){
                // discard all outputs from previous iteration
                for(int i=0; i<TRANSFORMATION(p)->outputs_count; i++){
                    gc_object_dereference(E, (gc_Object*)TRANSFORMATION(p)->outputs[i]);
                }
                predict_instruction_output(E, &prog->code[p], (char*)prog->constants, &dummy_objects_counter, TRANSFORMATION(p));
                for(int i=0; i<TRANSFORMATION(p)->outputs_count; i++){
                    vector_push(&stack, &TRANSFORMATION(p)->outputs[i]);
                    // output is refrenced by the transformation
                    gc_object_reference((gc_Object*)TRANSFORMATION(p)->outputs[i]);
                }
            } else {
                for(int i=0; i<TRANSFORMATION(p)->outputs_count; i++){
                    Dummy* output=TRANSFORMATION(p)->outputs[i];
                    vector_push(&stack, &output);
                }
            }
        }
    }
    if(print_optimisations){
        printf("Deduced flow chart:\n");
        print_transformations(prog->code, vector_get_data(&transformations), instructions_count);
        printf("\n");
    }

    // step 2: perform optimisations

    // replace jump to return or end instruction with return
    for(int pointer=0; INSTRUCTION(pointer)->type!=b_end; pointer++){
        if(INSTRUCTION(pointer)->type==b_jump && TRANSFORMATION(pointer)->inputs_count==1){
            int jump_destination=find_label(vector_get_data(&instructions), INSTRUCTION(pointer)->uint_argument);
            int instruction_after_label=jump_destination+1;
            // skip following labels and noops
            while(INSTRUCTION(instruction_after_label)->type==b_label || INSTRUCTION(instruction_after_label)->type==b_no_op){
                instruction_after_label++;
            }
            if(INSTRUCTION(instruction_after_label)->type==b_return || INSTRUCTION(instruction_after_label)->type==b_end){
                // remember what was the input to jump instruction
                Dummy* to_return=TRANSFORMATION(pointer)->inputs[0];
                gc_object_reference((gc_Object*)to_return);
                // remove destination label
                FILL_WITH_NO_OP(jump_destination, jump_destination);

                // change jump instruction to return
                transformation_deinit(E, TRANSFORMATION(pointer));
                INSTRUCTION(pointer)->type=b_return;
                transformation_from_instruction(TRANSFORMATION(pointer), INSTRUCTION(pointer));
                TRANSFORMATION(pointer)->inputs[0]=to_return;
            }
            break;
        }
    }
    for(int pointer=count_instructions((Instruction*)vector_get_data(&instructions))-1; pointer>=0; pointer--){
        // change calls to tail calls if possible
        if(INSTRUCTION(pointer)->type==b_call){
            if(INSTRUCTION(pointer+1)->type==b_end){
                INSTRUCTION(pointer)->type=b_tail_call;
            }
            if(INSTRUCTION(pointer+1)->type==b_return){
                INSTRUCTION(pointer)->type=b_tail_call;
                FILL_WITH_NO_OP(pointer+1, pointer+1);
            }
        }// replace variable lookup with stack operations 
        else if(INSTRUCTION(pointer)->type==b_set && INSTRUCTION(pointer+1)->type==b_discard
           && !INSTRUCTION(pointer)->bool_argument /* argument tells whether the variable is used in closure, we can't tell if the closure changes the variable*/
           && path_length(prog->code, pointer)<=2){// don't optimise nested paths like table.key, only single name paths
            if(print_optimisations){
                printf("Found a set Instruction\n");
                prog->code=(Instruction*)vector_get_data(&instructions);
                highlight_instructions(prog, '>', pointer-path_length(vector_get_data(&instructions), pointer)+1, pointer);
            }
            bool first_get_removal=true;
            bool used=false;
            for(int search_pointer=pointer+2; INSTRUCTION(search_pointer)->type!=b_end; search_pointer++){
                if(changes_flow(INSTRUCTION(search_pointer)->type) || finishes_program(INSTRUCTION(search_pointer)->type)){
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
                        prog->code=(Instruction*)vector_get_data(&instructions);
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
                        vector_insert(&instructions, pointer+1, &double_instruction);
                        Transformation double_transformation;
                        transformation_from_instruction(&double_transformation, &double_instruction);
                        Dummy* doubled=((Transformation*)vector_index(&transformations, pointer))->outputs[0];
                        double_transformation.inputs[0]=doubled;
                        double_transformation.outputs[0]=double_transformation.outputs[1]=doubled;
                        gc_object_reference((gc_Object*)doubled);
                        gc_object_reference((gc_Object*)doubled);
                        gc_object_reference((gc_Object*)doubled);
                        vector_insert(&transformations, pointer+1, &double_transformation);
                        vector_insert(&informations, pointer+1, vector_index(&informations, pointer));
                        search_pointer++;
                    }
                    // search for references to dummy object and replace them with the one
                    replace_dummies_in_transformations(E, 
                    (Instruction*)vector_get_data(&instructions), (Transformation*)vector_get_data(&transformations), vector_count(&instructions),
                    TRANSFORMATION(search_pointer)->outputs[0], 
                    TRANSFORMATION(pointer)->outputs[0]);

                    FILL_WITH_NO_OP(search_pointer-get_path_length+1, search_pointer);

                    if(search_pointer>=vector_count(&instructions)){
                        break;// after removing the path search_pointer points to b_end, if loop continued from this point it would get past the code's end
                    }
                }
            }
            if(!used){
                // the variable isn't used in it's own scope and in any closure, so it can be removed
                LOG_IF_ENABLED("Removing set Instruction:\n");
                replace_dummies_in_transformations(E, 
                (Instruction*)vector_get_data(&instructions), (Transformation*)vector_get_data(&transformations), vector_count(&instructions),
                TRANSFORMATION(pointer)->outputs[0], 
                TRANSFORMATION(pointer)->inputs[1]);
                FILL_WITH_NO_OP(pointer-path_length(CODE, pointer)+1, pointer);
            }
        }
    }
    // if an operation has no side effects and it's result is immediately discarded remove it
    for(int pointer=count_instructions((Instruction*)vector_get_data(&instructions))-1; pointer>=0; pointer--){
        if(INSTRUCTION(pointer)->type==b_discard){
            Dummy* discard_input=TRANSFORMATION(pointer)->inputs[0];
            // search for an instruction that outputs the discarded object
            for(int search=pointer-1; search>=0; search--){
                for(int o=0; o<TRANSFORMATION(search)->outputs_count; o++){
                    if(dummies_equal(TRANSFORMATION(search)->outputs[o], discard_input)){
                        goto found;
                    }
                }
                continue;
                found:
                if(TRANSFORMATION(search)->outputs_count==1 && !has_side_effects(INSTRUCTION(search), TRANSFORMATION(search))) {
                    Transformation* producer=TRANSFORMATION(search);
                    // discard inputs to producer
                    for(int i=0; i<producer->inputs_count; i++){
                        Instruction discard_instruction={b_discard};
                        vector_insert(&instructions, search, &discard_instruction);
                        Transformation discard_transformation;
                        transformation_from_instruction(&discard_transformation, &discard_instruction);
                        discard_transformation.inputs[0]=producer->inputs[i];
                        gc_object_reference((gc_Object*)discard_transformation.inputs[0]);
                        vector_insert(&transformations, search, &discard_transformation);
                        vector_insert(&informations, search, vector_index(&informations, search));
                        search++;
                        pointer++;
                    }
                    // remove producer and discard instruction
                    FILL_WITH_NO_OP(search, search)
                    FILL_WITH_NO_OP(pointer, pointer)
                }
                break;
            }
        }
    }

    if(print_optimisations){
        printf("Disconnected flow chart:\n");
        print_transformations(vector_get_data(&instructions), vector_get_data(&transformations), vector_count(&instructions));
        printf("\n");
    }

    // step 3: add swap instructions to ensure that objects are in right order
    // according to the flow chart made in the first step

    #define STACK(nth) ((Dummy**)vector_index(&stack, vector_count(&stack)-1-(nth)))
    for(int p=bytecode_progress_start(vector_get_data(&instructions), &progress_state); p!=-1; p=bytecode_progress_next(vector_get_data(&instructions), &progress_state)){
        if(p==0){
            // remove elements from dummy stack and push items from provided vector to it
            while(!vector_empty(&stack)){
                vector_pop(&stack);
            }
            for(int i=vector_count(&provided)-1; i>=0; i--){
                vector_push(&stack, vector_index(&provided, i));
            }
        }
        for(int i=0; i<gets_from_stack(*INSTRUCTION(p)); i++){
            Dummy* top=*STACK(i);
            Dummy* expected=TRANSFORMATION(p)->inputs[i];
            if(!dummies_compatible(top, expected)){
                int vector_depth=vector_count(&stack);
                for(int j=1; j<vector_depth; j++){
                    if(dummies_compatible(*STACK(j), expected)){
                        Instruction swap_instruction={b_swap};
                        swap_instruction.swap_argument.left=j;
                        swap_instruction.swap_argument.right=i;
                        Transformation swap_transformation;
                        transformation_from_instruction(&swap_transformation, &swap_instruction);
                        Dummy* any=new_dummy(E);
                        any->type=d_any;
                        for(int k=0; k<swap_transformation.inputs_count; k++){
                            if(k==swap_instruction.swap_argument.left || k==swap_instruction.swap_argument.right){
                                swap_transformation.inputs[k]=*STACK(k);
                                gc_object_reference((gc_Object*)swap_transformation.inputs[k]);
                            } else {
                                swap_transformation.inputs[k]=any;
                                gc_object_reference((gc_Object*)swap_transformation.inputs[k]);
                            }
                        }
                        predict_instruction_output(E, &swap_instruction, prog->constants, &dummy_objects_counter, &swap_transformation);
                        for(int k=0; k<swap_transformation.outputs_count; k++){
                            gc_object_reference((gc_Object*)swap_transformation.outputs[k]);
                        }
                        Dummy* temp=*STACK(swap_instruction.swap_argument.left);
                        *STACK(swap_instruction.swap_argument.left)=*STACK(swap_instruction.swap_argument.right);
                        *STACK(swap_instruction.swap_argument.right)=temp;
                        vector_insert(&instructions, p, &swap_instruction);
                        vector_insert(&transformations, p, &swap_transformation);
                        vector_insert(&informations, p, vector_index(&informations, p));
                        p=bytecode_progress_next(vector_get_data(&instructions), &progress_state);
                        break;
                    }
                }
            }
        }
        for(int j=0; j<gets_from_stack(*INSTRUCTION(p)); j++){
            vector_pop(&stack);
        }
        for(int i=0; i<pushes_to_stack(*INSTRUCTION(p)); i++){
            vector_push(&stack, &TRANSFORMATION(p)->outputs[i]);
        }
    }
    remove_no_ops(E, &instructions, &informations, &transformations);

    if(print_optimisations){
        printf("\nOptimised flow chart:\n");
        print_transformations(vector_get_data(&instructions), vector_get_data(&transformations), vector_count(&instructions));
    }

    // cleanup and moving data back from vectors to BytecodeProgram

    while(!vector_empty(&provided)){
        gc_object_dereference(E, (gc_Object*)*(Dummy**)vector_pop(&provided));
    }
    vector_deinit(&provided);
    vector_deinit(&stack);// dummy stack doesn't own any of it's objects
    for(int i=0; i<vector_count(&transformations); i++){
        transformation_deinit(E, TRANSFORMATION(i));
    }
    vector_deinit(&transformations);

    prog->code=(Instruction*)vector_get_data(&instructions);
    prog->information=(InstructionInformation*)vector_get_data(&informations);

    #undef FILL_WITH_NO_OP
    #undef INSTRUCTION
    #undef TRANSFORMATION
    #undef CODE
}

#undef LOG