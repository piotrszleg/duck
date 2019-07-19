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

int count_instructions(Instruction* code){
    int p=0;
    for(; code[p].type!=b_end; p++);
    return p;
}

void highlight_instructions(BytecodeProgram* program, char symbol, int start, int end){
    int pointer=0;
    while(program->code[pointer].type!=b_end){
        if(pointer>=start && pointer<=end){
            printf("%c ", symbol);
        } else {
            printf("  ");
        }
        print_instruction(program, program->code[pointer]);
        printf("\n");
        pointer++;
    }
}

typedef struct
{
    bool visited;
    Dummy** inputs;
    int inputs_count;
    Dummy** outputs;
    int outputs_count;
} Transformation;

static bool operand_has_side_effects(Dummy* dummy){
    return !dummy_is_typed(dummy)
        || (dummy_type(dummy)==t_table && dummy_type(dummy)==t_function);
}

bool has_side_effects(Instruction* instruction, Transformation* transformation) {
    switch(instruction->type) {
        case b_null:
        case b_load_int:
        case b_load_float:
        case b_load_string:
        case b_table_literal:
        case b_function_1:
        case b_function_2:
        case b_double:
            return false;
        case b_add:
        case b_subtract:
        case b_multiply:
        case b_divide:
        case b_divide_floor:
        case b_modulo:
        case b_add_int:
        case b_subtract_int:
        case b_multiply_int:
        case b_divide_int:
        case b_divide_floor_int:
        case b_modulo_int:
        case b_add_float:
        case b_subtract_float:
        case b_multiply_float:
        case b_divide_float:
        case b_add_string:
            return operand_has_side_effects(transformation->inputs[0]) 
            ||     operand_has_side_effects(transformation->inputs[1]);
        case b_binary:
            return operand_has_side_effects(transformation->inputs[1]) 
            ||     operand_has_side_effects(transformation->inputs[2]);
        case b_prefix:
            return operand_has_side_effects(transformation->inputs[1]);
        case b_minus:
        case b_minus_int:  
        case b_minus_float:
        case b_not:
            return operand_has_side_effects(transformation->inputs[0]);
        default: return true;
    }
}

Assumption* get_argument_assumption(BytecodeProgram* program, unsigned index){
    if(program->assumptions==NULL){
        return NULL;
    } else {
        return &program->assumptions[index];
    }
}

Assumption* get_upvalue_assumption(BytecodeProgram* program, Object identifier) {
    if(identifier.type!=t_string||program->assumptions==NULL){
        return NULL;
    }
    for(int i=0; i<program->upvalues_count; i++){
        char* upvalue_name=program->constants+program->upvalues[i];
        if(strcmp(upvalue_name, identifier.text)==0){
            return &program->assumptions[program->expected_arguments+i];
        }
    }
    return NULL;
}

Dummy* assumption_to_dummy(Executor* E, Assumption* assumption, unsigned* dummy_objects_counter){
    Dummy* result=new_dummy(E);
    if(assumption==NULL){
        result->type=d_any_type;
        result->id=(*dummy_objects_counter)++;
        return result;
    }
    switch(assumption->assumption_type){
        case a_constant:
            result->type=d_constant;
            result->constant_value=assumption->constant;
            break;
        case a_type:
            result->type=d_known_type;
            result->known_type=assumption->type;
            break;
        default: THROW_ERROR(BYTECODE_ERROR, "Incorrect assumption type.");
    }
    return result;
}

// function writes to transformation outputs the result of evaluating instruction with it's input
void predict_instruction_output(Executor* E, BytecodeProgram* program, Instruction* instr, char* constants, unsigned* dummy_objects_counter, Transformation* transformation){
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
            outputs[0]->id=(*dummy_objects_counter)++;
            outputs[0]->type=d_constant;
            outputs[0]->constant_value=null_const;
            reference(&outputs[0]->constant_value);
            return;
        case b_load_int:
            outputs[0]=new_dummy(E);
            outputs[0]->id=(*dummy_objects_counter)++;
            outputs[0]->type=d_constant;
            outputs[0]->constant_value=to_int(instr->int_argument);
            reference(&outputs[0]->constant_value);
            return;
        case b_load_float:
            outputs[0]=new_dummy(E);
            outputs[0]->id=(*dummy_objects_counter)++;
            outputs[0]->type=d_constant;
            outputs[0]->constant_value=to_float(instr->float_argument);
            reference(&outputs[0]->constant_value);
            return;
        case b_load_string:
            outputs[0]=new_dummy(E);
            outputs[0]->id=(*dummy_objects_counter)++;
            outputs[0]->type=d_constant;
            outputs[0]->constant_value=to_string(constants+instr->uint_argument);
            reference(&outputs[0]->constant_value);
            return;
        case b_table_literal:
            outputs[0]=new_dummy(E);
            outputs[0]->id=(*dummy_objects_counter)++;
            outputs[0]->type=d_known_type;
            outputs[0]->known_type=t_table;
            return;
        case b_function_1:
            outputs[0]=new_dummy(E);
            outputs[0]->id=(*dummy_objects_counter)++;
            outputs[0]->type=d_known_type;
            outputs[0]->known_type=t_function;
            return;
        case b_function_2:
            outputs[0]=inputs[0];
            return;
        case b_double:
            outputs[0]=inputs[0];
            outputs[1]=inputs[0];
            return;
        case b_push_to_top:
            outputs[instr->uint_argument]=inputs[0];
            outputs[0]=inputs[instr->uint_argument];
            for(int i=1; i<instr->uint_argument-1; i++){
                outputs[i]=outputs[instr->uint_argument-2-i];
            }
            return;
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
            return;
        }
        case b_get:
            if(inputs[0]->type==d_constant){
                outputs[0]=assumption_to_dummy(E, get_upvalue_assumption(program, inputs[0]->constant_value), dummy_objects_counter);
                if(outputs[0]->type==d_constant){
                    reference(&outputs[0]->constant_value);
                }
                return;
            }
            break;
        case b_set:
            outputs[0]=inputs[1];
            return;
        case b_binary:
        {
            if(inputs[0]->type==d_constant&&!operand_has_side_effects(inputs[1]) && !operand_has_side_effects(inputs[2])
            ){
                outputs[0]=new_dummy(E);
                if(inputs[0]->type==d_constant && inputs[1]->type==d_constant && inputs[2]->type==d_constant){
                    outputs[0]->type=d_constant;
                    outputs[0]->constant_value=operator(E, inputs[1]->constant_value, inputs[2]->constant_value, inputs[0]->constant_value.text);
                    reference(&outputs[0]->constant_value);
                } else {
                    outputs[0]->id=(*dummy_objects_counter)++;
                    outputs[0]->type=d_known_type;
                    outputs[0]->known_type=dummy_type(inputs[1]);
                }
                return;
            }
            break;
        }
        #define BINARY_OPERATOR(instruction, op) \
            case instruction: \
            { \
                if(!operand_has_side_effects(inputs[0]) && !operand_has_side_effects(inputs[1]) \
                ){ \
                    outputs[0]=new_dummy(E); \
                    if(inputs[0]->type==d_constant && inputs[1]->type==d_constant){ \
                        outputs[0]->type=d_constant; \
                        outputs[0]->constant_value=operator(E, inputs[0]->constant_value, inputs[1]->constant_value, op); \
                        reference(&outputs[0]->constant_value); \
                    } else { \
                        outputs[0]->id=(*dummy_objects_counter)++; \
                        outputs[0]->type=d_known_type; \
                        outputs[0]->known_type=dummy_type(inputs[0]); \
                    } \
                    return; \
                } \
                break; \
            }
        BINARY_OPERATOR(b_add, "+")
        BINARY_OPERATOR(b_subtract, "-")
        BINARY_OPERATOR(b_multiply, "*")
        BINARY_OPERATOR(b_divide, "/")
        BINARY_OPERATOR(b_divide_floor, "//")
        BINARY_OPERATOR(b_modulo, "%")
        BINARY_OPERATOR(b_add_int, "+")
        BINARY_OPERATOR(b_subtract_int, "-")
        BINARY_OPERATOR(b_multiply_int, "*")
        BINARY_OPERATOR(b_divide_int, "/")
        BINARY_OPERATOR(b_divide_floor_int, "//")
        BINARY_OPERATOR(b_modulo_int, "%")
        BINARY_OPERATOR(b_add_float, "+")
        BINARY_OPERATOR(b_subtract_float, "-")
        BINARY_OPERATOR(b_multiply_float, "*")
        BINARY_OPERATOR(b_divide_float, "/")
        BINARY_OPERATOR(b_add_string, "+")
        #undef BINARY_OPERATOR
        #define PREFIX_OPERATOR(instruction, op) \
            case instruction: \
            { \
                if(!operand_has_side_effects(inputs[0])){ \
                    outputs[0]=new_dummy(E); \
                    if(inputs[0]->type==d_constant){ \
                        outputs[0]->type=d_constant; \
                        outputs[0]->constant_value=operator(E, inputs[0]->constant_value, null_const, op); \
                        reference(&outputs[0]->constant_value); \
                    } else { \
                        outputs[0]->id=(*dummy_objects_counter)++; \
                        outputs[0]->type=d_known_type; \
                        outputs[0]->known_type=dummy_type(inputs[0]); \
                    } \
                    return; \
                } \
                break; \
            }
        PREFIX_OPERATOR(b_minus, "-")
        PREFIX_OPERATOR(b_minus_int, "-")
        PREFIX_OPERATOR(b_minus_float, "-")
        PREFIX_OPERATOR(b_not, "!")
        #undef PREFIX_OPERATOR
        default:;
    }
    for(int i=0; i<transformation->outputs_count; i++){
        outputs[i]=new_dummy(E);
        outputs[i]->id=(*dummy_objects_counter)++;
        outputs[i]->type=d_any_type;
    }
}

static void print_transformations(BytecodeProgram* program, Transformation* transformations, int transformations_count){
    printf("Instructions transformations:\n");
    for(int p=0; p<transformations_count; p++){
        printf("%s (", INSTRUCTION_NAMES[program->code[p].type]);
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

void replace_dummies_in_transformations(Executor* E, BytecodeProgram* program, Instruction* instructions, Transformation* transformations, char* constants, Dummy* to_replace, Dummy* replacement, unsigned* dummy_objects_counter){
    unsigned instructions_count=count_instructions(instructions)+1;
    for(int p=0; p<instructions_count; p++){
        bool input_modified=false;
        for(int i=0; i<transformations[p].inputs_count; i++){
            input_modified=input_modified||dummy_replace(E, &transformations[p].inputs[i], to_replace, replacement);
        }
        if(input_modified){
            Dummy** previous_outputs=malloc(sizeof(Dummy*)*transformations[p].outputs_count);
            for(int o=0; o<transformations[p].outputs_count; o++){
                previous_outputs[o]=transformations[p].outputs[o];
            }
            predict_instruction_output(E, program, &instructions[p], constants, dummy_objects_counter, &transformations[p]);
            for(int o=0; o<transformations[p].outputs_count; o++){
                if(previous_outputs[o]->type!=transformations[p].outputs[o]->type){
                    gc_object_reference((gc_Object*)transformations[p].outputs[o]);
                    replace_dummies_in_transformations(E, program, instructions, transformations, constants, previous_outputs[o], transformations[p].outputs[o], dummy_objects_counter);
                    gc_object_dereference(E, (gc_Object*)previous_outputs[o]);
                } else {
                    gc_object_dereference(E, (gc_Object*)transformations[p].outputs[o]);
                    transformations[p].outputs[o]=previous_outputs[o];
                    gc_object_reference((gc_Object*)transformations[p].outputs[o]);
                } 
            }
            free(previous_outputs);
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

typedef struct {
    vector branches;
    bool revisit;
    unsigned last;
    unsigned start;
}BytecodeIteratorState;

int bytecode_iterator_start(BytecodeIteratorState* state, Instruction* code, unsigned start){
    state->start=start;
    state->last=start;
    vector_init(&state->branches, sizeof(int), 8);
    state->revisit=false;
    return 0;
}

int bytecode_iterator_next(BytecodeIteratorState* state, Instruction* code){
    vector* branches=&state->branches;
    unsigned index=state->last;
    if(finishes_program(code[index].type)){
        if(state->revisit){
            state->revisit=false;
            index=state->start;
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

// true on success
bool constant_dummy_to_bytecode(Executor* E, Dummy* constant_dummy, unsigned position, vector* instructions, vector* transformations, vector* constants){
    Object object=constant_dummy->constant_value;
    #define LITERAL(type, argument) \
        { \
            Instruction replacement={type, argument}; \
            vector_insert(instructions, position, &replacement); \
        }
    switch(object.type){
        case t_null:
            LITERAL(b_null,)
            break;
        case t_float:
            LITERAL(b_load_float, .float_argument=object.float_value);
            break;
        case t_int:
            LITERAL(b_load_int, .int_argument=object.int_value);
            break;
        case t_string:{
            unsigned push_position=vector_count(constants);
            vector_insert_multiple(constants, push_position, object.text, strlen(object.text)+1);
            LITERAL(b_load_string, .uint_argument=push_position);
            break;
        }
        default: return false;
    }
    #undef LITERAL
    Transformation transformation;
    transformation_init(&transformation, 0, 1);
    transformation.outputs[0]=constant_dummy;
    gc_object_reference((gc_Object*)transformation.outputs[0]);
    vector_insert(transformations, position, &transformation);
    return true;
}

void optimise_bytecode(Executor* E, BytecodeProgram* program, bool print_optimisations){
    for(int i=0; i<program->sub_programs_count; i++){
        optimise_bytecode(E, &program->sub_programs[i], print_optimisations);
    }
    if(print_optimisations){
        print_bytecode_program(program);
    }

    // step 1: deduce how the objects will flow from one instruction to the next
    // also get a list of externally provided objects

    int instructions_count=count_instructions(program->code)+1;
    vector provided, stack, transformations, instructions, informations, constants;
    vector_init(&provided, sizeof(Dummy*), 64);
    vector_init(&stack, sizeof(Dummy*), 128);
    vector_init(&transformations, sizeof(Transformation), instructions_count);
    vector_extend(&transformations, instructions_count);
    vector_from(&instructions, sizeof(Instruction), program->code, instructions_count);
    vector_from(&informations, sizeof(InstructionInformation), program->information, instructions_count);
    vector_from(&constants, sizeof(char), program->constants, program->constants_size);
    #define INSTRUCTION(nth) ((Instruction*)vector_index(&instructions, (nth)))
    #define TRANSFORMATION(nth) ((Transformation*)vector_index(&transformations, (nth)))
    #define CODE ((Instruction*)vector_get_data(&instructions))
    #define REBUILD_PROGRAM \
        program->code=(Instruction*)vector_get_data(&instructions); \
        program->information=(InstructionInformation*)vector_get_data(&informations); \
        program->constants_size=vector_count(&constants); \
        program->constants=(char*)vector_get_data(&constants);
    #define HIGHLIGHT(symbol, start, end) \
        if(print_optimisations){ \
            REBUILD_PROGRAM \
            highlight_instructions(program, symbol, start, end); \
        }
    #define FILL_WITH_NO_OP(start, end) \
        HIGHLIGHT('-', start, end); \
        for(int i=start; i<=end; i++) { \
            transformation_deinit(E, TRANSFORMATION(i)); \
            INSTRUCTION(i)->type=b_no_op; \
        }
    unsigned int dummy_objects_counter=0;

    BytecodeIteratorState progress_state;
    for(int p=bytecode_iterator_start(&progress_state, program->code, 0); p!=-1; p=bytecode_iterator_next(&progress_state, program->code)){
        if(p==0){
            for(int i=vector_count(&provided)-1; i>=0; i--){
                vector_push(&stack, vector_index(&provided, i));
            }
        }
        if(!TRANSFORMATION(p)->visited){
            Transformation transformation;
            if(carries_stack(program->code[p].type)){
                // jump_not takes one item from the stack as a predicate
                if(program->code[p].type==b_jump_not){
                    transformation_init(&transformation, vector_count(&stack), vector_count(&stack)-1);
                } else {
                    transformation_init(&transformation, vector_count(&stack), vector_count(&stack));
                }
            } else {
                transformation_from_instruction(&transformation, &program->code[p]);
            }
            
            for(int i=0; i<transformation.inputs_count; i++){
                if(!vector_empty(&stack)){
                    transformation.inputs[i]=*(Dummy**)vector_pop(&stack);
                } else {
                    /*Dummy* dummy=new_dummy(E);
                    dummy->id=dummy_objects_counter++;
                    dummy->type=d_any_type;*/
                    Dummy* dummy=assumption_to_dummy(E, get_argument_assumption(program, vector_count(&provided)), &dummy_objects_counter);
                    vector_push(&provided, &dummy);
                    transformation.inputs[i]=dummy;
                    // in this case dummy is also referenced by provided
                    gc_object_reference((gc_Object*)transformation.inputs[i]);
                }
                // input is referenced by the transformation
                gc_object_reference((gc_Object*)transformation.inputs[i]);
            }
            predict_instruction_output(E, program, &program->code[p], (char*)program->constants, &dummy_objects_counter, &transformation);
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
                predict_instruction_output(E, program, &program->code[p], (char*)program->constants, &dummy_objects_counter, TRANSFORMATION(p));
                for(int i=0; i<TRANSFORMATION(p)->outputs_count; i++){
                    vector_push(&stack, &TRANSFORMATION(p)->outputs[i]);
                    // output is referenced by the transformation
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
        print_transformations(program, vector_get_data(&transformations), instructions_count);
        printf("\n");
    }

    // step 2: perform optimisations

    // replace jump to return or end instruction with return
    for(int pointer=0; INSTRUCTION(pointer)->type!=b_end; pointer++){
        if(INSTRUCTION(pointer)->type==b_jump && TRANSFORMATION(pointer)->inputs_count==1){
            int jump_destination=find_label(vector_get_data(&instructions), INSTRUCTION(pointer)->uint_argument);
            int instruction_after_label=jump_destination+1;
            // skip following labels and no-ops
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
    for(int pointer=count_instructions((Instruction*)vector_get_data(&instructions))-1; pointer>=1; pointer--){
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
           && (INSTRUCTION(pointer-1)->type==b_load_string || INSTRUCTION(pointer-1)->type==b_load_int)){// don't optimise nested paths like table.key, only single name paths
            if(print_optimisations){
                printf("Found a set Instruction\n");
                program->code=(Instruction*)vector_get_data(&instructions);
                REBUILD_PROGRAM
                highlight_instructions(program, '>', pointer-path_length(vector_get_data(&instructions), pointer)+1, pointer);
            }
            bool first_get_removal=true;
            bool used=false;
            for(int get_search=bytecode_iterator_start(&progress_state, program->code, 0); get_search!=-1; get_search=bytecode_iterator_next(&progress_state, program->code)){
                if(changes_scope(INSTRUCTION(get_search)->type)){
                    // we optimised all gets in this scope so the variable isn't needed anymore
                    break;
                }
                REBUILD_PROGRAM
                if(INSTRUCTION(get_search)->type==b_get && instructions_equal(program, *INSTRUCTION(get_search-1), *INSTRUCTION(pointer-1))){
                    if(print_optimisations){
                        printf("Found a corresponding get Instruction\n");
                        program->code=(Instruction*)vector_get_data(&instructions);
                        highlight_instructions(program, '>', get_search-path_length(CODE, pointer)+1, get_search);
                    }

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
                        get_search=bytecode_iterator_next(&progress_state, program->code);
                    }
                    // search for references to dummy object and replace them with the one
                    Dummy* to_replace=TRANSFORMATION(get_search)->outputs[0];
                    gc_object_reference((gc_Object*)to_replace);
                    FILL_WITH_NO_OP(get_search-1, get_search);
                    REBUILD_PROGRAM
                    replace_dummies_in_transformations(E, program,
                    (Instruction*)vector_get_data(&instructions), (Transformation*)vector_get_data(&transformations), (char*)vector_get_data(&constants),
                    to_replace, TRANSFORMATION(pointer)->outputs[0], &dummy_objects_counter);
                    gc_object_dereference(E, (gc_Object*)to_replace);

                    if(get_search>=vector_count(&instructions)){
                        break;// after removing the path get_search points to b_end, if loop continued from this point it would get past the code's end
                    }
                }
            }
            if(!used){
                // the variable isn't used in it's own scope and in any closure, so it can be removed
                LOG_IF_ENABLED("Removing set Instruction:\n");
                Dummy* to_replace=TRANSFORMATION(pointer)->outputs[0];
                gc_object_reference((gc_Object*)to_replace);
                Dummy* replacing=TRANSFORMATION(pointer)->inputs[1];
                gc_object_reference((gc_Object*)replacing);
                FILL_WITH_NO_OP(pointer-1, pointer);
                REBUILD_PROGRAM
                replace_dummies_in_transformations(E, program, 
                (Instruction*)vector_get_data(&instructions), (Transformation*)vector_get_data(&transformations), (char*)vector_get_data(&constants),
                to_replace, replacing, &dummy_objects_counter);
                gc_object_dereference(E, (gc_Object*)to_replace);
                gc_object_dereference(E, (gc_Object*)replacing);
            }
        }
    }    
    for(int pointer=count_instructions((Instruction*)vector_get_data(&instructions))-1; pointer>=0; pointer--){
        // if an operation has no side effects and it's result is immediately discarded remove it
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
                if((TRANSFORMATION(search)->outputs_count==1 || INSTRUCTION(search)->type==b_double)
                && !has_side_effects(INSTRUCTION(search), TRANSFORMATION(search))) {
                    LOG_IF_ENABLED("Removing operation which result is immediately discarded:\n")
                    Transformation* producer=TRANSFORMATION(search);
                    // discard inputs to producer
                    int to_discard=producer->inputs_count;
                    if(INSTRUCTION(search)->type==b_double){
                        to_discard--;
                    }
                    for(int i=0; i<to_discard; i++){
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
        // if operation has no side effect and it's output is known replace it with it's result literal
        else if(TRANSFORMATION(pointer)->outputs_count==1 
        && TRANSFORMATION(pointer)->outputs[0]->type==d_constant
        && !instruction_is_literal(INSTRUCTION(pointer)->type)
        && !has_side_effects(INSTRUCTION(pointer), TRANSFORMATION(pointer))){
            Transformation producer=*TRANSFORMATION(pointer);
            if(constant_dummy_to_bytecode(E, TRANSFORMATION(pointer)->outputs[0], pointer+1, &instructions, &transformations, &constants)){
                LOG_IF_ENABLED("Replacing operation with it's result:\n")
                // discard inputs to producer
                for(int i=0; i<producer.inputs_count; i++){
                    Instruction discard_instruction={b_discard};
                    vector_insert(&instructions, pointer+1, &discard_instruction);
                    Transformation discard_transformation;
                    transformation_from_instruction(&discard_transformation, &discard_instruction);
                    discard_transformation.inputs[0]=producer.inputs[i];
                    gc_object_reference((gc_Object*)discard_transformation.inputs[0]);
                    vector_insert(&transformations, pointer+1, &discard_transformation);
                    vector_insert(&informations, pointer+1, vector_index(&informations, pointer+1));
                }
                FILL_WITH_NO_OP(pointer, pointer)
                pointer+=producer.inputs_count+1;
            }
        } 
        // typed variants of operators and prefixes
        else {
            #define OPERATOR_VARIANT(base_instruction, type_name) \
            if(INSTRUCTION(pointer)->type==base_instruction \
                && dummy_type(TRANSFORMATION(pointer)->inputs[0])==t_##type_name \
                && dummy_type(TRANSFORMATION(pointer)->inputs[1])==t_##type_name){ \
                    INSTRUCTION(pointer)->type=base_instruction##_##type_name; \
                }
            OPERATOR_VARIANT(b_add, int)
            OPERATOR_VARIANT(b_add, float)
            OPERATOR_VARIANT(b_add, string)
            OPERATOR_VARIANT(b_subtract, int)
            OPERATOR_VARIANT(b_subtract, float)
            OPERATOR_VARIANT(b_multiply, int)
            OPERATOR_VARIANT(b_multiply, float)
            OPERATOR_VARIANT(b_divide, int)
            OPERATOR_VARIANT(b_divide, float)
            OPERATOR_VARIANT(b_divide_floor, int)
            OPERATOR_VARIANT(b_modulo, int)
            #define PREFIX_VARIANT(base_instruction, type_name) \
                if(INSTRUCTION(pointer)->type==base_instruction \
                && dummy_type(TRANSFORMATION(pointer)->inputs[0])==t_##type_name){ \
                    INSTRUCTION(pointer)->type==base_instruction##_##type_name; \
                }
            OPERATOR_VARIANT(b_minus, int)
            OPERATOR_VARIANT(b_minus, float)
        }
    }

    if(print_optimisations){
        printf("Disconnected flow chart:\n");
        REBUILD_PROGRAM
        print_transformations(program, vector_get_data(&transformations), vector_count(&instructions));
        printf("\n");
    }

    // step 3: add swap instructions to ensure that objects
    // are in right order according to the flow chart made in the first step

    #define STACK(nth) ((Dummy**)vector_index(&stack, vector_count(&stack)-1-(nth)))
    for(int p=bytecode_iterator_start(&progress_state, vector_get_data(&instructions), 0); p!=-1; p=bytecode_iterator_next(&progress_state, vector_get_data(&instructions))){
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
                        REBUILD_PROGRAM
                        predict_instruction_output(E, program, &swap_instruction, program->constants, &dummy_objects_counter, &swap_transformation);
                        for(int k=0; k<swap_transformation.outputs_count; k++){
                            gc_object_reference((gc_Object*)swap_transformation.outputs[k]);
                        }
                        Dummy* temp=*STACK(swap_instruction.swap_argument.left);
                        *STACK(swap_instruction.swap_argument.left)=*STACK(swap_instruction.swap_argument.right);
                        *STACK(swap_instruction.swap_argument.right)=temp;
                        vector_insert(&instructions, p, &swap_instruction);
                        vector_insert(&transformations, p, &swap_transformation);
                        vector_insert(&informations, p, vector_index(&informations, p));
                        p=bytecode_iterator_next(&progress_state, vector_get_data(&instructions));
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
    // discard objects that aren't used anywhere
    // this shouldn't happen in a correctly assembled program
    // but it's a way to get extra safety
    /*while(!vector_empty(&stack)){
        unsigned insert_position=vector_count(&instructions)-1;
        Instruction instruction={b_discard};
        vector_insert(&instructions, insert_position, &instruction);
        Transformation transformation;
        transformation_init(&transformation, 1, 0);
        transformation.inputs[0]=*(Dummy**)vector_pop(&stack);
        gc_object_reference((gc_Object*)transformation.inputs[0]);
        vector_insert(&transformations, insert_position, &transformation);
        // take information from end instruction
        vector_push(&informations, vector_top(&informations));
    } */
    
    remove_no_ops(E, &instructions, &informations, &transformations);

    if(print_optimisations){
        printf("\nFinal flow chart:\n");
        REBUILD_PROGRAM
        print_transformations(program, vector_get_data(&transformations), vector_count(&instructions));
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

    REBUILD_PROGRAM

    #undef FILL_WITH_NO_OP
    #undef INSTRUCTION
    #undef TRANSFORMATION
    #undef CODE
}

#undef LOG