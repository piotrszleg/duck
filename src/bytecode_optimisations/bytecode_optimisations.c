#include "bytecode_optimisations.h"

bool is_instruction_of_type(vector* instructions, uint index, InstructionType expected_type){
    return vector_index_instruction(instructions, index)->type==expected_type;
}

bool is_optimisable_set(vector* instructions, int instruction_index){
    return is_instruction_of_type(instructions, instruction_index, b_set) 
    && is_instruction_of_type(instructions, instruction_index+1, b_discard)
    /* bool argument tells whether the variable is used in closure, we can't tell if the closure changes the variable */
    && !vector_index_instruction(instructions, instruction_index)->bool_argument
    && 
    (
        /* don't optimise nested paths like table.key, only single name paths */
        is_instruction_of_type(instructions, instruction_index-1, b_load_string) 
        || is_instruction_of_type(instructions, instruction_index-1, b_load_int)
    );
}

bool is_compatible_get(BytecodeManipulation* manipulation, int set_index, int instruction_index){
    return vector_index_instruction(manipulation->instructions, instruction_index)->type==b_get 
    && instructions_equal(*vector_index_instruction(manipulation->instructions, instruction_index-1), 
                          *vector_index_instruction(manipulation->instructions, set_index-1), 
                          vector_get_data(manipulation->constants));
                
}

bool insert_double(BytecodeManipulation* manipulation, uint index, Dummy* doubled){
    Instruction double_instruction={b_double};
    Transformation double_transformation;
    transformation_from_instruction(&double_transformation, &double_instruction);
    double_transformation.inputs[0]=doubled;
    double_transformation.outputs[0]=double_transformation.outputs[1]=doubled;
    heap_object_reference((HeapObject*)doubled);
    heap_object_reference((HeapObject*)doubled);
    heap_object_reference((HeapObject*)doubled);
    return insert_instruction(manipulation, index, &double_instruction, &double_transformation);
}

void optimise_get_instructions(Executor* E,
    BytecodeManipulation* manipulation,
    vector* instructions, 
    vector* transformations, 
    vector* constants, 
    uint set_location,
    bool print_optimisations
){
    bool first_get_removal=true;
    int inner_scope_depth=0;// used to check when the scope was left
    
    BytecodeIterator bytecode_iterator;
    for(int get_search=bytecode_iterator_start(&bytecode_iterator, vector_get_data(instructions), set_location); 
        get_search!=-1;
        get_search=bytecode_iterator_next(&bytecode_iterator, vector_get_data(instructions))
    ){
        if(vector_index_instruction(instructions, get_search)->type==b_enter_scope){
            inner_scope_depth++;
        } else if(vector_index_instruction(instructions, get_search)->type==b_leave_scope){
            inner_scope_depth--;
        }
        if(inner_scope_depth<0){
            // we optimised all gets in this path in this scope
            get_search=bytecode_iterator_next_path(&bytecode_iterator, vector_get_data(instructions));
            if(get_search==-1){
                break;
            }
        }
        if(is_compatible_get(manipulation, set_location, get_search)){
            begin_recording_change(manipulation, "Removing get instruction.", (int[]){set_location, get_search, -1});
            if(first_get_removal){
                fill_with_no_op(manipulation, set_location+1, set_location+1);
                first_get_removal=false;
            } else{
                Dummy* doubled=vector_index_transformation(transformations, set_location)->outputs[0];
                if(insert_double(manipulation, set_location+1, doubled)) {
                    get_search=bytecode_iterator_next(&bytecode_iterator, vector_get_data(instructions));// skip inserted instruction
                }
            }
            // search for references to dummy object output by get and replace them with the one passed to set
            Dummy* to_replace=vector_index_transformation(transformations, get_search)->outputs[0];
            heap_object_reference((HeapObject*)to_replace);
            fill_with_no_op(manipulation, get_search-1, get_search);
            replace_dummies_in_transformations(manipulation, to_replace, vector_index_transformation(transformations, set_location)->outputs[0]);
            heap_object_dereference(E, (HeapObject*)to_replace);
            end_recording_change(manipulation);
        }
    }
}
    
// replace variable lookup with stack operations if possible
void variable_lookup_optimisation(
    Executor* E, 
    BytecodeProgram* program, 
    BytecodeManipulation* manipulation,
    vector* instructions, 
    vector* transformations, 
    vector* constants, 
    bool print_optimisations) {
    for(int set_search=vector_count(instructions)-1; set_search>=1; set_search--){
        if(is_optimisable_set(instructions, set_search)){
            optimise_get_instructions(E, manipulation, instructions, transformations, constants, set_search, print_optimisations);
            // the variable isn't used in it's own scope and in any closure, so the set instruction can be removed
            begin_recording_change(manipulation, "Removing useless set instruction.", (int[]){set_search, -1});
            Dummy* to_replace=vector_index_transformation(transformations, set_search)->outputs[0];
            dummy_reference(to_replace);
            Dummy* replacing=vector_index_transformation(transformations, set_search)->inputs[1];
            dummy_reference(replacing);
            fill_with_no_op(manipulation, set_search-1, set_search);
            // replace output of set instruction with it's input
            replace_dummies_in_transformations(manipulation, to_replace, replacing);
            heap_object_dereference(E, (HeapObject*)to_replace);
            heap_object_dereference(E, (HeapObject*)replacing);
            end_recording_change(manipulation);
        }
    }
}

// replace jump to return or end instruction with return
// this allows more tail call optimisations to happen
void jump_to_return(
    BytecodeManipulation* manipulation,
    vector* instructions, 
    vector* transformations){
    for(int pointer=0; vector_index_instruction(instructions, pointer)->type!=b_end; pointer++){
        if(vector_index_instruction(instructions, pointer)->type==b_jump && vector_index_transformation(transformations, pointer)->inputs_count==1){
            int jump_destination=find_label(vector_get_data(instructions), vector_index_instruction(instructions, pointer)->uint_argument);
            int instruction_after_label=jump_destination+1;
            // skip following labels and no-ops
            while(vector_index_instruction(instructions, instruction_after_label)->type==b_label 
                || vector_index_instruction(instructions, instruction_after_label)->type==b_no_op){
                instruction_after_label++;
            }
            if(vector_index_instruction(instructions, instruction_after_label)->type==b_return 
                || vector_index_instruction(instructions, instruction_after_label)->type==b_end){
                
                // remember what was the input to jump instruction
                Dummy* to_return=vector_index_transformation(transformations, pointer)->inputs[0];
                heap_object_reference((HeapObject*)to_return);
                // remove destination label
                fill_with_no_op(manipulation, jump_destination, jump_destination);

                // change jump instruction to return
                transformation_deinit(manipulation->executor, vector_index_transformation(transformations, pointer));
                vector_index_instruction(instructions, pointer)->type=b_return;
                transformation_from_instruction(vector_index_transformation(transformations, pointer), vector_index_instruction(instructions, pointer));
                vector_index_transformation(transformations, pointer)->inputs[0]=to_return;
            }
            break;
        }
    }
}

// replace normal calls with tail calls if possible
bool tail_calls(BytecodeManipulation* manipulation, vector* instructions, vector* transformations, int pointer, bool print_optimisations){
    if(is_instruction_of_type(instructions, pointer, b_call)){
        InstructionType next_type=vector_index_instruction(instructions, pointer+1)->type;
        if(next_type==b_end || next_type==b_return){
            Transformation tail_call_transformation;
            transformation_init(&tail_call_transformation, vector_index_transformation(transformations, pointer)->inputs_count, 0);
            for(int i=0; i<tail_call_transformation.inputs_count; i++){
                tail_call_transformation.inputs[i]=vector_index_transformation(transformations, pointer)->inputs[i];
                heap_object_reference((HeapObject*)tail_call_transformation.inputs[i]);
            }
            transformation_deinit(manipulation->executor, vector_index_transformation(transformations, pointer));
            *vector_index_transformation(transformations, pointer)=tail_call_transformation;
            vector_index_instruction(instructions, pointer)->type=b_tail_call;
            if(next_type==b_end){
                vector_index_transformation(transformations, pointer+1)->inputs[0]=new_any_dummy(manipulation->executor);
                heap_object_reference((HeapObject*)vector_index_transformation(transformations, pointer+1)->inputs);
            } else {
                // remove the following return instruction
                fill_with_no_op(manipulation, pointer+1, pointer+1);
            }
            return true;
        }
    }
    return false;
}

bool inline_native_calls(BytecodeManipulation* manipulation, vector* instructions, vector* transformations, int pointer, bool print_optimisations) {
    if((is_instruction_of_type(instructions, pointer, b_call) || is_instruction_of_type(instructions, pointer, b_tail_call))
        && vector_index_transformation(transformations, pointer)->inputs[0]->type==d_constant) {
            Transformation* call_transformation=vector_index_transformation(transformations, pointer);
            Object function=call_transformation->inputs[0]->constant_value;
            if(function.type==t_function && function.fp->ftype==f_native){
                bool is_tail_call=vector_index_instruction(instructions, pointer)->type==b_tail_call;
                uint arguments_count=vector_index_instruction(instructions, pointer)->uint_argument;

                Instruction call_1_instruction={is_tail_call?b_native_tail_call_1:b_native_call_1, .uint_argument=(uint)(long)function.fp->native_pointer};
                Transformation call_1_transformation;
                transformation_init(&call_1_transformation, call_transformation->inputs_count-1, is_tail_call?0:1);
                for(int i=0; i<call_1_transformation.inputs_count; i++){
                    call_1_transformation.inputs[i]=call_transformation->inputs[i+1];
                    heap_object_reference((HeapObject*)call_1_transformation.inputs[i]);
                }

                Instruction call_2_instruction={is_tail_call?b_native_tail_call_2:b_native_call_2, .uint_argument=arguments_count};
                Transformation call_2_transformation;
                transformation_from_instruction(&call_2_transformation, &call_2_instruction);
                if(!is_tail_call){
                    call_2_transformation.outputs[0]=call_transformation->outputs[0];
                }
                // discard function object input
                pointer+=insert_discard(manipulation, call_transformation->inputs[0], pointer);
                fill_with_no_op(manipulation, pointer, pointer);
                insert_instruction(manipulation, pointer, &call_1_instruction, &call_1_transformation);
                insert_instruction(manipulation, pointer+1, &call_2_instruction, &call_2_transformation);

                return true;
            }
    }
    return false;
}

/*bool flatten_table(){
    // if only operations on table_literal output are table_set_keep, table_set and table_get
    // and it doesn't have overriding symbols
        // for each table_set_keep belonging to the literal find corresponding table_get and replace it's output with table_set_keep second input
        // using replace_dummies_in_transformations
    // remove table literal
}*/

// if an operation has no side effects and it's result is immediately discarded removes it
bool remove_useless_operations(BytecodeManipulation* manipulation, vector* instructions, vector* transformations, int pointer, bool print_optimisations){
    if(vector_index_instruction(instructions, pointer)->type==b_discard){
        Dummy* discard_input=vector_index_transformation(transformations, pointer)->inputs[0];
        int producer_index=0;
        if(find_dummy_producer(transformations, discard_input, pointer-1, &producer_index)){
            if((vector_index_transformation(transformations, producer_index)->outputs_count==1 
                || is_instruction_of_type(instructions, producer_index, b_double))
                && instruction_is_constant(vector_index_instruction(instructions, producer_index), 
                                           vector_index_transformation(transformations, producer_index))
                ) {
                begin_recording_change(manipulation, "Removing operation which result is immediately discarded", (int[]){producer_index, -1});
                // discard inputs to the producer if it's not a double instruction
                if(!is_instruction_of_type(instructions, producer_index, b_double)){
                    uint instructions_written=discard_transformation_inputs(manipulation, producer_index);
                    producer_index+=instructions_written;
                    pointer+=instructions_written;
                }
                // remove producer and discard instruction
                fill_with_no_op(manipulation, producer_index, producer_index);
                fill_with_no_op(manipulation, pointer, pointer);
                end_recording_change(manipulation);
                return true;
            } else {
                return false;
            }
        }
    }
    return false;
}


// if operation has no side effect and it's output is known it is replaced with it's result literal
bool fold_constants(
    BytecodeManipulation* manipulation,
    vector* instructions, 
    vector* transformations, 
    vector* constants, 
    int* pointer, 
    bool print_optimisations
){
    Instruction* instruction=vector_index_instruction(instructions, *pointer);
    Transformation* transformation=vector_index_transformation(transformations, *pointer);
    if(transformation->outputs_count==1 
    && transformation->outputs[0]->type==d_constant
    && !instruction_is_literal(instruction->type)
    && instruction_is_constant(instruction, transformation)
    ){
        if(constant_dummy_to_bytecode(manipulation->executor, transformation->outputs[0], *pointer+1, instructions, transformations, constants)){
            begin_recording_change(manipulation, "Replacing instruction with its result", (int[]){*pointer, -1});
            (*pointer)+=discard_transformation_inputs(manipulation, *pointer);
            fill_with_no_op(manipulation, *pointer, *pointer);
            end_recording_change(manipulation);
            return true;
        }
    }
    return false;
}

bool typed_instructions(vector* instructions, vector* transformations, int pointer){
    #define OPERATOR_VARIANT(base_instruction, type_name) \
    if(vector_index_instruction(instructions, pointer)->type==base_instruction \
        && dummy_type(vector_index_transformation(transformations, pointer)->inputs[0])==t_##type_name \
        && dummy_type(vector_index_transformation(transformations, pointer)->inputs[1])==t_##type_name){ \
            vector_index_instruction(instructions, pointer)->type=base_instruction##_##type_name; \
            return true; \
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
        if(vector_index_instruction(instructions, pointer)->type==base_instruction \
        && dummy_type(vector_index_transformation(transformations, pointer)->inputs[0])==t_##type_name){ \
            begin_recording_change(manipulation, (int[]){pointer, -1}); \
            vector_index_instruction(instructions, pointer)->type==base_instruction##_##type_name; \
            end_recording_change(manipulation); \
            return true; \
        }
    OPERATOR_VARIANT(b_minus, int)
    OPERATOR_VARIANT(b_minus, float)
    return false;
}

void fill_unreachable_transformations(Executor* E, Instruction* instructions, vector* transformations){
    for(int p=0; p<vector_count(transformations); p++){
        Transformation* transformation=vector_index_transformation(transformations, p);
        if(!transformation->visited) {
            transformation_from_instruction(transformation, &instructions[p]);
            for(int i=0; i<transformation->inputs_count; i++){
                transformation->inputs[i]=new_any_dummy(E);
            }
            for(int i=0; i<transformation->outputs_count; i++){
                transformation->outputs[i]=new_any_dummy(E);
            }
        }
    }
}

void revisit_transformation(
    Executor* E, 
    BytecodeProgram* program, 
    vector* stack, 
    uint* dummy_objects_counter, 
    Transformation* transformation,
    Instruction* instruction
){
    bool inputs_changed=false;
    for(int i=0; i<transformation->inputs_count; i++){
        Dummy* from_stack=*(Dummy**)vector_pop(stack);
        Dummy* expected=(Dummy*)transformation->inputs[i];
        if(!dummies_equal(expected, from_stack)){
            if(dummy_contains(from_stack, expected)){
                // change previous input to the one found on the stack
                dummy_dereference(E, expected);
                dummy_reference(from_stack);
                transformation->inputs[i]=from_stack;
                inputs_changed=true;
            } else if(!dummy_contains(expected, from_stack)){
                Dummy* or_dummy=new_or_dummy(E, expected, from_stack, dummy_objects_counter);
                // or_dummy references both inputs
                dummy_dereference(E, expected);// expected is no longer referenced by transformation itself
                transformation->inputs[i]=or_dummy;
                // replace_dummies_in_transformations(manipulation, expected, or_dummy);
                inputs_changed=true;
            }
        }
    }
    if(inputs_changed){
        // discard all outputs from previous iteration
        for(int i=0; i<transformation->outputs_count; i++){
            dummy_dereference(E, transformation->outputs[i]);
        }
        predict_instruction_output(E, program, instruction, 
            (char*)program->constants, dummy_objects_counter, transformation);
        for(int i=0; i<transformation->outputs_count; i++){
            vector_push(stack, &transformation->outputs[i]);
        }
    } else {
        for(int i=0; i<transformation->outputs_count; i++){
            Dummy* output=transformation->outputs[i];
            vector_push(stack, &output);
        }
    }
}

// Important note: stack doesn't own dummies (it doesn't affect their ref_count)
void visit_transformation(
    Executor* E,
    vector* stack,
    vector* provided,
    BytecodeProgram* program,
    uint* dummy_objects_counter,
    Instruction* instruction,
    Transformation* transformation
){
    if(carries_stack(instruction->type)){
        // jump_not takes one item from the stack as a predicate
        if(instruction->type==b_jump_not){
            transformation_init(transformation, vector_count(stack), vector_count(stack)-1);
        } else {
            transformation_init(transformation, vector_count(stack), vector_count(stack));
        }
    } else {
        transformation_from_instruction(transformation, instruction);
    }
    
    for(int i=0; i<transformation->inputs_count; i++){
        if(!vector_empty(stack)){
            transformation->inputs[i]=*(Dummy**)vector_pop(stack);
            // dummy is referenced by the transformation
            dummy_reference(transformation->inputs[i]);
        } else {
            Dummy* dummy=assumption_to_dummy(E, get_argument_assumption(program, vector_count(provided)), dummy_objects_counter);
            vector_push(provided, &dummy);
            // dummy returned from assumption_to_dummy already has ref_count of 1
            // so this reference goes to the transformation
            transformation->inputs[i]=dummy;
            // dummy is also referenced by provided
            dummy_reference(transformation->inputs[i]);
        }
    }
    predict_instruction_output(E, program, instruction, (char*)program->constants, dummy_objects_counter, transformation);
    for(int i=0; i<transformation->outputs_count; i++){
        vector_push(stack, &transformation->outputs[i]);
        // output is only referenced by the transformation
    }
    transformation->visited=true;
}

void generate_flow_chart(
    Executor* E, 
    vector* provided, 
    vector* stack, 
    BytecodeProgram* program, 
    vector* transformations,
    uint* dummy_objects_counter
){
    int p;
    BytecodeIterator bytecode_iterator;
    BYTECODE_FOREACH_PATH(bytecode_iterator, p, program->code){
        if(p==0){
            for(int i=vector_count(provided)-1; i>=0; i--){
                vector_push(stack, vector_index(provided, i));
            }
        }
        if(!vector_index_transformation(transformations, p)->visited){
            visit_transformation(E, stack, provided, program, dummy_objects_counter, 
                &program->code[p], vector_index_transformation(transformations, p));
        } else {
            revisit_transformation(E, program, stack, dummy_objects_counter, 
                vector_index_transformation(transformations, p), &program->code[p]);
        }
    }
    fill_unreachable_transformations(E, program->code, transformations);
}

bool insert_swap(BytecodeManipulation* manipulation, int index, int left, int right, Dummy* left_dummy, Dummy* right_dummy){
    Instruction swap_instruction={b_swap};
    swap_instruction.swap_argument.left=left;
    swap_instruction.swap_argument.right=right;
    Transformation swap_transformation;
    transformation_from_instruction(&swap_transformation, &swap_instruction);
    Dummy* any=new_any_dummy(manipulation->executor);
    for(int k=0; k<swap_transformation.inputs_count; k++){
        if(k==left){
            swap_transformation.inputs[k]=left_dummy;
        } else if(k==right){
            swap_transformation.inputs[k]=right_dummy;
        } else {
            swap_transformation.inputs[k]=any;
        }
        dummy_reference(swap_transformation.inputs[k]);
    }
    dummy_dereference(manipulation->executor, any);
    predict_instruction_output(manipulation->executor, 
                               manipulation->program, 
                               &swap_instruction, 
                               manipulation->program->constants, 
                               manipulation->dummy_objects_counter, 
                               &swap_transformation);
    return insert_instruction(manipulation, index, &swap_instruction, &swap_transformation);
}

void add_swaps(
    BytecodeManipulation* manipulation,
    vector* provided,
    vector* stack,
    vector* instructions,
    vector* transformations
){
    // add swap instructions to ensure that objects
    // are in right order according to the flow chart made in the first step
    
    #define STACK(nth) (vector_index_dummy(stack, vector_count(stack)-1-(nth)))
    BytecodeIterator bytecode_iterator;
    int p=0;
    BYTECODE_FOREACH_PATH(bytecode_iterator, p, vector_get_data(instructions)) {
        if(p==0){
            // remove elements from dummy stack and push items from provided vector to it
            while(!vector_empty(stack)){
                vector_pop(stack);
            }
            for(int i=vector_count(provided)-1; i>=0; i--){
                vector_push(stack, vector_index(provided, i));
            }
        }
        for(int i=0; i<vector_index_transformation(transformations, p)->inputs_count; i++){
            Dummy* top=*STACK(i);
            Dummy* expected=vector_index_transformation(transformations, p)->inputs[i];
            if(!dummies_compatible(top, expected)){
                int vector_depth=vector_count(stack);
                for(int j=1; j<vector_depth; j++){
                    if(dummies_compatible(*STACK(j), expected)){
                        int swap_left=j;
                        int swap_right=i;
                        if(insert_swap(manipulation, p, swap_left, swap_right, *STACK(swap_left), *STACK(swap_right))){
                            p=bytecode_iterator_next(&bytecode_iterator, vector_get_data(instructions));
                        }
                        // perform the swap on dummy stack
                        Dummy* temp=*STACK(swap_left);
                        *STACK(swap_left)=*STACK(swap_right);
                        *STACK(swap_right)=temp;
                        break;
                    }
                }
            }
        }
        for(int j=0; j<vector_index_transformation(transformations, p)->inputs_count; j++){
            vector_pop(stack);
        }
        for(int i=0; i<vector_index_transformation(transformations, p)->outputs_count; i++){
            vector_push(stack, &vector_index_transformation(transformations, p)->outputs[i]);
        }
    }
    #undef STACK
}

void remove_swaps(BytecodeManipulation* manipulation){
    for(int p=0; p<vector_count(manipulation->instructions); p++){
        if(vector_index_instruction(manipulation->instructions, p)->type==b_swap){
            fill_with_no_op(manipulation, p, p);
        }
    }
}

void discard_excess_objects(BytecodeManipulation* manipulation, vector* stack){
    while(!vector_empty(stack)){
        Instruction instruction={b_discard};
        Transformation transformation;
        transformation_init(&transformation, 1, 0);
        transformation.inputs[0]=*(Dummy**)vector_pop(stack);
        heap_object_reference((HeapObject*)transformation.inputs[0]);
        insert_instruction(manipulation, vector_count(manipulation->instructions)-1, &instruction, &transformation);
    }
}

void assert_flow_chart_correctness(
    vector* provided,
    vector* stack,
    vector* transformations
){
    #define CHECK_DUMMY_VECTOR(vector) \
        for(int i=0; i<vector_count(vector); i++){ \
            dummy_assert_correctness(*vector_index_dummy(vector, i)); \
        }
    CHECK_DUMMY_VECTOR(provided)
    CHECK_DUMMY_VECTOR(stack)
    #undef CHECK_DUMMY_VECTOR

    for(int t=0; t<vector_count(transformations); t++){
        Transformation* transformation=vector_index_transformation(transformations, t);
        // assert(transformation->visited);
        for(int i=0; i<transformation->inputs_count; i++){
            dummy_assert_correctness(transformation->inputs[i]);
        }
        for(int i=0; i<transformation->outputs_count; i++){
            dummy_assert_correctness(transformation->outputs[i]);
        }
    }
}

void optimise_bytecode(Executor* E, BytecodeProgram* program, bool print_optimisations){
    for(int i=0; i<program->sub_programs_count; i++){
        optimise_bytecode(E, program->sub_programs[i], print_optimisations);
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
    unsigned int dummy_objects_counter=0;
    #define CHECK \
        IF_DEBUGGING(assert_flow_chart_correctness(&provided, &stack, &transformations))
    generate_flow_chart(E, &provided, &stack, program, &transformations, &dummy_objects_counter);
    CHECK

    // step 2: perform optimisations

    BytecodeManipulation manipulation;
    manipulation.instructions=&instructions;
    manipulation.transformations=&transformations;
    manipulation.informations=&informations;
    manipulation.constants=&constants;
    manipulation.executor=E;
    manipulation.dummy_objects_counter=&dummy_objects_counter;
    manipulation.program=program;
    manipulation.print_optimisations=print_optimisations;

    initialize_recording(&manipulation);

    // remove swap instructions because they are only making code more confusing and prevent some optimisations
    // they will be readded later in the most optimal way
    remove_swaps(&manipulation);
    CHECK
    /*
        As you probably noticed some variables are passed two times, in manipulation struct
        and as function arguments. The general rule is if function uses some variable directly 
        in it's body it is also passed as argument. 
        This is to avoid unpacking the manipulation struct back into variables in the function. 
        If it is only passed futher to other function like insert_instruction 
        then it is stored in BytecodeManipulation struct.
        This way calls to these functions are shorter and more readable.

        This is the best I could do to make this entire file readable for a human.
    */
    if(E->options.optimise_variable_lookup) {
        variable_lookup_optimisation(E, program, &manipulation, &instructions, &transformations, &constants, print_optimisations);
    }
    CHECK
    if(E->options.inline_functions) {
        inline_functions(E, program, &manipulation, &instructions, &informations, &transformations, &constants, print_optimisations);
    }
    CHECK
    if(E->options.optimise_jump_to_return){
        jump_to_return(&manipulation, &instructions, &transformations);
    }
    CHECK
    for(int pointer=vector_count(&instructions)-2; pointer>=1; pointer--){
        // here short circuit evaluation is used so that if one of optimisations succeeds the rest is not performed on the same instruction
        allow_unused_variable((void*)(long)(// prevent emitting "value computed is not used" warning
           (E->options.inline_native_calls
                && inline_native_calls(&manipulation, &instructions, &transformations, pointer, print_optimisations))
        || (E->options.optimise_tail_calls
                && tail_calls(&manipulation, &instructions, &transformations, pointer, print_optimisations))
        || (E->options.remove_useless_operations 
                && remove_useless_operations(&manipulation, &instructions, &transformations, pointer, print_optimisations))
        || (E->options.fold_constants
                && fold_constants(&manipulation, &instructions, &transformations, &constants, &pointer, print_optimisations))
        || (E->options.use_typed_instructions
                && typed_instructions(&instructions, &transformations, pointer))
        ));
    }
    CHECK
    discard_excess_objects(&manipulation, &stack);
    CHECK

    begin_recording_change(&manipulation, "Adding swap instructions.", NULL);
    add_swaps(&manipulation, &provided, &stack, &instructions, &transformations);
    end_recording_change(&manipulation);

    CHECK

    remove_no_ops(E, &instructions, &informations, &transformations);

    CHECK

    finish_recording(&manipulation);

    // cleanup and moving data back from vectors to BytecodeProgram
    while(!vector_empty(&provided)){
        dummy_dereference(E, *(Dummy**)vector_pop(&provided));
    }
    CHECK
    vector_deinit(&provided);
    vector_deinit(&stack);// dummy stack doesn't own any of it's objects
    for(int i=0; i<vector_count(&transformations); i++){
        transformation_deinit(E, vector_index_transformation(&transformations, i));
    }
    vector_deinit(&transformations);

    program->code=(Instruction*)vector_get_data(&instructions);
    program->information=(InstructionInformation*)vector_get_data(&informations);
    program->constants_size=vector_count(&constants);
    program->constants=(char*)vector_get_data(&constants);

    #undef FILL_WITH_NO_OP
}