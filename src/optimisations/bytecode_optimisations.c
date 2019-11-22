#include "bytecode_optimisations.h"

#define LOG_IF_ENABLED(message, ...) \
    if(print_optimisations){ \
        printf(message, ##__VA_ARGS__); \
    }

unsigned count_instructions(Instruction* code){
    unsigned p=0;
    for(; code[p].type!=b_end; p++);
    return p;
}

void highlight_instructions(Instruction* instructions, void* constants, char symbol, int start, int end){
    int pointer=0;
    while(instructions[pointer].type!=b_end){
        if(pointer>=start && pointer<=end){
            printf("%c ", symbol);
        } else {
            printf("  ");
        }
        print_instruction(instructions[pointer], constants);
        printf("\n");
        pointer++;
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
    if(assumption==NULL){
        return new_any_type_dummy(E, dummy_objects_counter);
    }
    switch(assumption->assumption_type){
        case a_constant:
            return new_constant_dummy(E, assumption->constant, dummy_objects_counter);
        case a_type:
            return new_known_type_dummy(E, assumption->type, dummy_objects_counter); 
        default:
            THROW_ERROR(BYTECODE_ERROR, "Incorrect assumption type.");
            return NULL;
    }
}

bool instruction_is_constant(Instruction* instruction, Transformation* transformation) {
    switch(instruction->type) {
        case b_null:
        case b_load_int:
        case b_load_float:
        case b_load_string:
        case b_table_literal:
        case b_function_1:
        case b_function_2:
        case b_double:
        case b_get:
        case b_discard:
            return true;
        #define BINARY(instruction, op) \
            case instruction: \
                return operator_predict_result(dummy_type(transformation->inputs[0]), dummy_type(transformation->inputs[1]), op)!=tu_unknown;
        #define PREFIX(instruction, op) \
            case instruction: \
                return operator_predict_result(t_null, dummy_type(transformation->inputs[0]), op)!=tu_unknown;
        OPERATOR_INSTRUCTIONS
        #undef BINARY
        #undef PREFIX
        case b_binary:
            return transformation->inputs[0]->type==d_constant 
            && transformation->inputs[0]->constant_value.type==t_string
            && operator_predict_result(dummy_type(transformation->inputs[1]), dummy_type(transformation->inputs[2]), transformation->inputs[0]->constant_value.text)!=tu_unknown;
        case b_prefix:
            return transformation->inputs[0]->type==d_constant 
            && transformation->inputs[0]->constant_value.type==t_string
            && operator_predict_result(tu_null, dummy_type(transformation->inputs[1]), transformation->inputs[0]->constant_value.text)!=tu_unknown;
        default: return false;
    }
}

// function writes to transformation outputs the result of evaluating instruction with it's input
void predict_instruction_output(Executor* E, BytecodeProgram* program, Instruction* instruction, char* constants, unsigned* dummy_objects_counter, Transformation* transformation){
    Dummy** outputs=transformation->outputs;
    Dummy** inputs=transformation->inputs;
    if(carries_stack(instruction->type)){
        int i=0;
        // jump_not takes one item from the stack as a predicate
        // so it needs to be skipped
        if(instruction->type==b_jump_not){
            i++;
        }
        for(; i<transformation->inputs_count; i++){
            outputs[transformation->inputs_count-1-i]=inputs[i];
        }
        return;
    }
    switch (instruction->type){
        case b_null:
            outputs[0]=new_constant_dummy(E, null_const, dummy_objects_counter);
            return;
        case b_load_int:
            outputs[0]=new_constant_dummy(E, to_int(instruction->int_argument), dummy_objects_counter);
            return;
        case b_load_float:
            outputs[0]=new_constant_dummy(E, to_float(instruction->float_argument), dummy_objects_counter);
            return;
        case b_load_string:
            outputs[0]=new_constant_dummy(E, to_string(constants+instruction->uint_argument), dummy_objects_counter);
            return;
        case b_table_literal:
            outputs[0]=new_known_type_dummy(E, t_table, dummy_objects_counter);
            return;
        case b_function_1:
        {
            #define FUNCTION_CONSTANT_DUMMY 0
            #if FUNCTION_CONSTANT_DUMMY
                Object f;
                function_init(E, &f);
                f.fp->enclosing_scope=null_const;
                f.fp->ftype=f_bytecode;
                f.fp->arguments_count=instruction->function_argument.arguments_count;
                f.fp->variadic=instruction->function_argument.is_variadic;
                outputs[0]=new_constant_dummy(E, f, dummy_objects_counter);
            #else
                outputs[0]=new_known_type_dummy(E, t_function, dummy_objects_counter);
            #endif
            return;
        }
        case b_function_2:
            #if FUNCTION_CONSTANT_DUMMY
                if(inputs[0]->type==d_constant){
                    HeapObject* program=(HeapObject*)program->sub_programs[instruction->uint_argument];
                    inputs[0]->constant_value.fp->source_pointer=program;
                    heap_object_reference(program);
                }
            #endif
            outputs[0]=inputs[0];
            return;
        case b_double:
            outputs[0]=inputs[0];
            outputs[1]=inputs[0];
            return;
        case b_push_to_top:
            outputs[instruction->uint_argument]=inputs[0];
            outputs[0]=inputs[instruction->uint_argument];
            for(int i=1; i<instruction->uint_argument-1; i++){
                outputs[i]=outputs[instruction->uint_argument-2-i];
            }
            return;
        case b_swap:
        {
            for(int i=0; i<transformation->outputs_count; i++){
                outputs[transformation->outputs_count-1-i]=inputs[i];
            }
            int left=transformation->outputs_count-1-instruction->swap_argument.left;
            int right=transformation->outputs_count-1-instruction->swap_argument.right;
            Dummy* temp=outputs[left];
            outputs[left]=outputs[right];
            outputs[right]=temp;
            return;
        }
        case b_get:
            if(inputs[0]->type==d_constant){
                outputs[0]=assumption_to_dummy(E, get_upvalue_assumption(program, inputs[0]->constant_value), dummy_objects_counter);
                return;
            }
            break;
        case b_set:
            outputs[0]=inputs[1];
            return;
        case b_table_set_keep:
            outputs[0]=inputs[2];
            return;
        case b_binary:
        {
            if(inputs[0]->type==d_constant && inputs[0]->constant_value.type==t_string) {
                ObjectTypeOrUnknown prediction=operator_predict_result(dummy_type(inputs[1]), dummy_type(inputs[2]), inputs[0]->constant_value.text);
                if(prediction!=tu_unknown){
                    if(inputs[0]->type==d_constant && inputs[1]->type==d_constant && inputs[2]->type==d_constant){
                        Object operator_result=operator(E, inputs[1]->constant_value, inputs[2]->constant_value, inputs[0]->constant_value.text);
                        if(is_unhandled_error(E, operator_result)){
                            set(E, operator_result, to_string("handled"), to_int(1));
                            dereference(E, &operator_result);
                            outputs[0]=new_any_type_dummy(E, dummy_objects_counter);
                        } else {
                            outputs[0]=new_constant_dummy(E, operator_result, dummy_objects_counter);
                        }
                    } else {
                        outputs[0]=new_known_type_dummy(E, (ObjectType)prediction, dummy_objects_counter);
                    }
                    return;
                }
            }
            break;
        }
        #define BINARY(instruction, op) \
            case instruction: \
            { \
                ObjectTypeOrUnknown prediction=operator_predict_result(dummy_type(inputs[0]), dummy_type(inputs[1]), op); \
                if(prediction!=tu_unknown){ \
                    if(inputs[0]->type==d_constant && inputs[1]->type==d_constant){ \
                        Object operator_result=operator(E, inputs[0]->constant_value, inputs[1]->constant_value, op); \
                        if(is_unhandled_error(E, operator_result)){ \
                            set(E, operator_result, to_string("handled"), to_int(1)); \
                            dereference(E, &operator_result); \
                            outputs[0]=new_any_type_dummy(E, dummy_objects_counter); \
                        } else { \
                            outputs[0]=new_constant_dummy(E, operator_result, dummy_objects_counter); \
                        } \
                    } else { \
                        outputs[0]=new_known_type_dummy(E, (ObjectType)prediction, dummy_objects_counter); \
                    } \
                    return; \
                } \
                break; \
            }
        #define PREFIX(instruction, op) \
            case instruction: \
            { \
                ObjectTypeOrUnknown prediction=operator_predict_result(tu_null, dummy_type(inputs[0]), op); \
                if(prediction!=tu_unknown){ \
                    if(inputs[0]->type==d_constant) { \
                        outputs[0]=new_constant_dummy(E, operator(E, inputs[0]->constant_value, null_const, op), dummy_objects_counter); \
                    } else { \
                        outputs[0]=new_known_type_dummy(E, (ObjectType)prediction, dummy_objects_counter); \
                    } \
                    return; \
                } \
                break; \
            }
        OPERATOR_INSTRUCTIONS
        #undef BINARY
        #undef PREFIX
        default:;
    }
    for(int i=0; i<transformation->outputs_count; i++){
        outputs[i]=new_any_type_dummy(E, dummy_objects_counter);
    }
}

static void print_transformations(Instruction* instructions, Transformation* transformations){
    printf("Instructions transformations:\n");
    unsigned instructions_count=count_instructions(instructions)+1;
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
    heap_object_reference((HeapObject*)transformation.outputs[0]);
    vector_insert(transformations, position, &transformation);
    return true;
}

#define VECTOR_INDEX_FUNCTION(type, postfix) \
    type* vector_index_##postfix(vector* v, int index) { \
        return (type*)vector_index(v, index); \
    }

VECTOR_INDEX_FUNCTION(Instruction, instruction)
VECTOR_INDEX_FUNCTION(Transformation, transformation)
VECTOR_INDEX_FUNCTION(InstructionInformation, information)

void remove_no_ops(Executor* E, vector* instructions, vector* informations, vector* transformations){
    int block_start=0;
    bool inside_block=false;
    for(int p=0; p<vector_count(instructions); p++){
        if(inside_block){
            if(vector_index_instruction(instructions, p)->type!=b_no_op || vector_index_instruction(instructions, p)->type==b_end){
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
        if(vector_index_instruction(instructions, p)->type==b_no_op){
            block_start=p;
            inside_block=true;
        }
    }
}

typedef struct {
    BytecodeProgram* program;
    Executor* executor;
    vector* transformations;
    vector* instructions;
    vector* constants;
    vector* informations;
    bool print_optimisations;
    unsigned* dummy_objects_counter;
} BytecodeManipulation;

void replace_dummies_in_transformations(BytecodeManipulation* manipulation, Dummy* to_replace, Dummy* replacement){
    unsigned instructions_count=count_instructions(vector_get_data(manipulation->instructions))+1;
    for(int p=0; p<instructions_count; p++){
        bool input_modified=false;
        Transformation* transformation=vector_index_transformation(manipulation->transformations, p);
        for(int i=0; i<transformation->inputs_count; i++){
            input_modified=input_modified||dummy_replace(manipulation->executor, &transformation->inputs[i], to_replace, replacement);
        }
        if(input_modified){
            Dummy** previous_outputs=malloc(sizeof(Dummy*)*transformation->outputs_count);
            for(int o=0; o<transformation->outputs_count; o++){
                previous_outputs[o]=transformation->outputs[o];
            }
            predict_instruction_output(manipulation->executor, manipulation->program, vector_index_instruction(manipulation->instructions, p), 
                vector_get_data(manipulation->constants), manipulation->dummy_objects_counter, transformation);
            for(int o=0; o<transformation->outputs_count; o++){
                if(previous_outputs[o]->type!=transformation->outputs[o]->type){
                    heap_object_reference((HeapObject*)transformation->outputs[o]);
                    replace_dummies_in_transformations(manipulation, previous_outputs[o], transformation->outputs[o]);
                    heap_object_dereference(manipulation->executor, (HeapObject*)previous_outputs[o]);
                } else {
                    heap_object_dereference(manipulation->executor, (HeapObject*)transformation->outputs[o]);
                    transformation->outputs[o]=previous_outputs[o];
                    heap_object_reference((HeapObject*)transformation->outputs[o]);
                } 
            }
            free(previous_outputs);
        }
        for(int i=0; i<transformation->outputs_count; i++){
            dummy_replace(manipulation->executor, &transformation->outputs[i], to_replace, replacement);
        }
    }
}

void fill_with_no_op(BytecodeManipulation* manipulation, unsigned start, unsigned end) {
    if(manipulation->print_optimisations){
        highlight_instructions(vector_get_data(manipulation->instructions), vector_get_data(manipulation->constants), '-', start, end);
    }
    for(int i=start; i<=end; i++) {
        transformation_deinit(manipulation->executor, vector_index_transformation(manipulation->transformations, i));
        vector_index_instruction(manipulation->instructions, i)->type=b_no_op;
    }
}

// returns true if the result is inserting the instrcution and not changing b_no_op to it
bool insert_instruction(BytecodeManipulation* manipulation, unsigned index, Instruction* instruction, Transformation* transformation) {
    if(vector_index_instruction(manipulation->instructions, index)->type==b_no_op) {
        *vector_index_instruction(manipulation->instructions, index)=*instruction;
        *vector_index_transformation(manipulation->transformations, index)=*transformation;
        return false;
    } else {
        vector_insert(manipulation->instructions, index, instruction);
        vector_insert(manipulation->transformations, index, transformation);
        // copy information from previous instruction
        vector_insert(manipulation->informations, index, vector_index(manipulation->informations, index-1));
        return true;
    }
    if(manipulation->print_optimisations){
        highlight_instructions(vector_get_data(manipulation->instructions), vector_get_data(manipulation->constants), '+', index, index);
    }
}

// replace variable lookup with stack operations if possible
void stack_usage_optimisations(
    Executor* E, 
    BytecodeProgram* program, 
    BytecodeManipulation* manipulation,
    vector* instructions, 
    vector* transformations, 
    vector* constants, 
    bool print_optimisations) {
    for(int set_search=count_instructions((Instruction*)vector_get_data(instructions))-1; set_search>=1; set_search--){
        if(vector_index_instruction(instructions, set_search)->type==b_set && vector_index_instruction(instructions, set_search+1)->type==b_discard
           && !vector_index_instruction(instructions, set_search)->bool_argument /* argument tells whether the variable is used in closure, we can't tell if the closure changes the variable*/
           && (vector_index_instruction(instructions, set_search-1)->type==b_load_string || vector_index_instruction(instructions, set_search-1)->type==b_load_int)){// don't optimise nested paths like table.key, only single name paths
            if(print_optimisations) {
                printf("Found a set Instruction\n");
                highlight_instructions(vector_get_data(instructions), vector_get_data(constants), 
                    '>', set_search-1, set_search);
            }
            bool first_get_removal=true;
            bool used=false;
            int inner_scope_depth=0;// used to check when the scope was left

            BytecodeIterator progress_state;
            int get_search;
            BYTECODE_FOR(progress_state, get_search, vector_get_data(instructions)){
                if(vector_index_instruction(instructions, get_search)->type==b_enter_scope){
                    inner_scope_depth++;
                } else if(vector_index_instruction(instructions, get_search)->type==b_leave_scope){
                    inner_scope_depth--;
                }
                if(inner_scope_depth<0){
                    // we optimised all gets in this scope so the variable isn't needed anymore
                    break;
                }
                if(vector_index_instruction(instructions, get_search)->type==b_get 
                && instructions_equal(*vector_index_instruction(instructions, get_search-1), *vector_index_instruction(instructions, set_search-1), vector_get_data(constants))){
                    if(print_optimisations){
                        printf("Found a corresponding get Instruction\n");
                        highlight_instructions(vector_get_data(instructions), vector_get_data(constants), 
                            '>', get_search-1, get_search);
                    }
                    if(first_get_removal){
                        LOG_IF_ENABLED("Removing discard Instruction after set Instruction:\n");
                        fill_with_no_op(manipulation, set_search+1, set_search+1);
                        first_get_removal=false;
                    } else{
                        Instruction double_instruction={b_double};
                        Transformation double_transformation;
                        transformation_from_instruction(&double_transformation, &double_instruction);
                        Dummy* doubled=((Transformation*)vector_index(transformations, set_search))->outputs[0];
                        double_transformation.inputs[0]=doubled;
                        double_transformation.outputs[0]=double_transformation.outputs[1]=doubled;
                        heap_object_reference((HeapObject*)doubled);
                        heap_object_reference((HeapObject*)doubled);
                        heap_object_reference((HeapObject*)doubled);
                        if(insert_instruction(manipulation, set_search+1, &double_instruction, &double_transformation)) {
                            get_search=bytecode_iterator_next(&progress_state, vector_get_data(instructions));// skip inserted instruction
                        }
                    }
                    // search for references to dummy object output by get and replace them with the one passed to set
                    Dummy* to_replace=vector_index_transformation(transformations, get_search)->outputs[0];
                    heap_object_reference((HeapObject*)to_replace);
                    fill_with_no_op(manipulation, get_search-1, get_search);
                    replace_dummies_in_transformations(manipulation, to_replace, vector_index_transformation(transformations, set_search)->outputs[0]);
                    heap_object_dereference(E, (HeapObject*)to_replace);

                    if(get_search>=vector_count(instructions)){
                        break;// after removing the path get_search points to b_end, if loop continued from this point it would get past the code's end
                    }
                }
            }
            if(!used){
                // the variable isn't used in it's own scope and in any closure, so the set instruction can be removed
                LOG_IF_ENABLED("Removing set Instruction:\n");
                Dummy* to_replace=vector_index_transformation(transformations, set_search)->outputs[0];
                heap_object_reference((HeapObject*)to_replace);
                Dummy* replacing=vector_index_transformation(transformations, set_search)->inputs[1];
                heap_object_reference((HeapObject*)replacing);
                fill_with_no_op(manipulation, set_search-1, set_search);
                // replace output of set instruction with it's input
                replace_dummies_in_transformations(manipulation, to_replace, replacing);
                heap_object_dereference(E, (HeapObject*)to_replace);
                heap_object_dereference(E, (HeapObject*)replacing);
            }
        }
    }
}

bool is_subprogram(const BytecodeProgram* presumed_subprogram, const BytecodeProgram* program) {
    if(presumed_subprogram==program){
        return true;
    } else {
        for(int i=0; i<program->sub_programs_count; i++) {
            if(is_subprogram(presumed_subprogram, program->sub_programs[i])){
                return true;
            }
        }
        return false;
    }
}

// inserts transformations with any dummies as inputs and outputs
void insert_any_transformations(BytecodeManipulation* manipulation, unsigned position, unsigned count) {
    for(int i=position; i<position+count; i++){
        Transformation transformation;
        transformation_from_instruction(&transformation, vector_index_instruction(manipulation->instructions, i));
        for(int j=0; j<transformation.inputs_count; j++){
            transformation.inputs[j]=new_any_dummy(manipulation->executor);
            heap_object_reference((HeapObject*)transformation.inputs[j]);
        }
        for(int j=0; j<transformation.outputs_count; j++){
            transformation.outputs[j]=new_any_dummy(manipulation->executor);
            heap_object_reference((HeapObject*)transformation.outputs[j]);
        }
        vector_insert(manipulation->transformations, i, &transformation);
    }
}

void proccess_inlined_function_code(
    BytecodeManipulation* manipulation, 
    int start, 
    int end, 
    int inlined_function_constants_start, 
    int inlined_function_subprograms_start, 
    unsigned last_label_index, 
    Dummy* output){
    last_label_index++;
    int function_end_label=last_label_index;
    for(int j=start; j<end; j++){
        Instruction* instruction=vector_index_instruction(manipulation->instructions, j);
        switch (instruction->type)
        {
            case b_label:
            case b_jump:
                // shift argument, save the last label index to increment last_label index of the main program
                instruction->uint_argument+=last_label_index;
                break;
            case b_load_string:
                instruction->uint_argument+=inlined_function_constants_start;
                break;
            case b_function_2:
                instruction->uint_argument+=inlined_function_subprograms_start;
                break;
            case b_return: {
                Transformation* transformation=vector_index_transformation(manipulation->transformations, j);
                Dummy* return_input=transformation->outputs[0];
                // return_input will become input and output of jump instruction
                heap_object_reference((HeapObject*)return_input);
                heap_object_reference((HeapObject*)return_input);
                instruction->type=b_jump;
                instruction->uint_argument=function_end_label;
                // return transformation doesn't have an output
                // so we need to create new transformation for jump instruction
                transformation_deinit(manipulation->executor, transformation);
                transformation_init(transformation, 1, 1);
                transformation->inputs[0]=transformation->outputs[0]=return_input;
                break;
            }
            case b_tail_call: {
                instruction->type=b_call;
                Instruction jump_instruction={b_jump, .uint_argument=function_end_label};
                Transformation jump_transformation;
                transformation_init(&jump_transformation, 1, 1);
                jump_transformation.inputs[0]=jump_transformation.outputs[0]=vector_index_transformation(manipulation->transformations, j)->outputs[0];
                heap_object_reference((HeapObject*)jump_transformation.inputs[0]);
                heap_object_reference((HeapObject*)jump_transformation.outputs[0]);
                insert_instruction(manipulation, j+1, &jump_instruction, &jump_transformation);
                end++;
            }
            default:;
        }
    }
    Instruction label_instruction={b_label, .uint_argument=function_end_label};
    Transformation label_transformation;
    transformation_init(&label_transformation, 1, 1);
    // these dummies were referenced earlier
    label_transformation.outputs[0]=label_transformation.inputs[0]=output;
    insert_instruction(manipulation, end, &label_instruction, &label_transformation);
}

void inline_functions(
    Executor* E,
    BytecodeProgram* program,
    BytecodeManipulation* manipulation,
    vector* instructions, 
    vector* informations, 
    vector* transformations,
    vector* constants,
    bool print_optimisations) {
    unsigned last_label_index=0;
    for(int i=0; i<vector_count(instructions); i++){
        Instruction* instruction=vector_index_instruction(instructions, i);
        if(instruction->type==b_label){
            last_label_index=MAX(last_label_index, instruction->uint_argument);
        }
    }
    for(int i=0; i<vector_count(instructions); i++){
        if((vector_index_instruction(instructions, i)->type==b_call || vector_index_instruction(instructions, i)->type==b_tail_call)) {
            Dummy* input=vector_index_transformation(transformations, i)->inputs[0];
            if(input->type!=d_constant
                || input->constant_value.type!=t_function 
                || input->constant_value.fp->ftype!=f_bytecode){
                continue;// skip function if it isn't a bytecode function
            }
            Function* function=input->constant_value.fp;
            BytecodeProgram* function_program=(BytecodeProgram*)function->source_pointer;
            int provided_arguments=vector_index_transformation(transformations, i)->inputs_count-1;// -1 to skip the function object
            bool arguments_count_matches= provided_arguments==function->arguments_count
                                          || (function->variadic && provided_arguments>function->arguments_count);
            bool scope_matches= function->enclosing_scope.type==t_null 
                                || is_subprogram(function_program, program);
            // TODO: copy over subprograms of inlined function
            if(arguments_count_matches && scope_matches){
                Dummy* function_input=vector_index_transformation(transformations, i)->inputs[0];
                heap_object_reference((HeapObject*)function_input);
                Dummy* call_output=vector_index_transformation(transformations, i)->outputs[0];
                // call output is referenced as input and output to the label ending inlined function
                heap_object_reference((HeapObject*)call_output);
                heap_object_reference((HeapObject*)call_output);

                // remove call instruction and discard the function input
                fill_with_no_op(manipulation, i, i);
                Instruction discard_instruction={b_discard};
                Transformation discard_transformation;
                transformation_from_instruction(&discard_transformation, &discard_instruction);
                discard_transformation.inputs[0]=function_input;
                insert_instruction(manipulation, i, &discard_instruction, &discard_transformation);

                // insert instructions and informations from inlined function
                unsigned code_length=count_instructions(function_program->code);
                vector_insert_multiple(instructions, i+1, function_program->code, code_length);
                vector_insert_multiple(informations, i+1, function_program->information, code_length);
                // the inlined function isn't optimised in this optimisation session (it probably was optimised earlier anyways)
                // so all transformations of inlined function are just filled with any dummies
                // TODO: change these to proper transformations
                insert_any_transformations(manipulation, i+1, code_length);
                int inlined_function_constants_start=vector_count(constants);
                vector_insert_multiple(constants, inlined_function_constants_start, function_program->constants, function_program->constants_size);

                int inlined_function_subprograms_start=program->sub_programs_count;
                // merge subprograms of the inlined function with subrprograms of main function
                if(function_program->sub_programs_count!=0){
                    program->sub_programs_count+=function_program->sub_programs_count;
                    program->sub_programs=realloc(program->sub_programs, program->sub_programs_count*sizeof(BytecodeProgram*));
                    for(int i=0; i<function_program->sub_programs_count; i++) {
                        program->sub_programs[inlined_function_subprograms_start+i]=function_program->sub_programs[i];
                        heap_object_reference((HeapObject*)function_program->sub_programs[i]);
                    }
                }

                proccess_inlined_function_code(manipulation, i+1, i+1+code_length, 
                    inlined_function_constants_start, inlined_function_subprograms_start, last_label_index, call_output);

                if(print_optimisations){
                    printf("Inlining function:\n");
                    highlight_instructions(vector_get_data(instructions), vector_get_data(manipulation->constants), 
                        '+', i+1, i+code_length+1);
                }
                i+=code_length+1;
            }
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
    if(vector_index_instruction(instructions, pointer)->type==b_call){
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
                fill_with_no_op(manipulation, pointer+1, pointer+1);
            }
            return true;
        }
    }
    return false;
}

bool insert_discard(BytecodeManipulation* manipulation, Dummy* discard_input, uint index) {
    Instruction discard_instruction={b_discard};
    Transformation discard_transformation;
    transformation_from_instruction(&discard_transformation, &discard_instruction);
    discard_transformation.inputs[0]=discard_input;
    heap_object_reference((HeapObject*)discard_transformation.inputs[0]);
    return insert_instruction(manipulation, index, &discard_instruction, &discard_transformation);
}

// returns the number of inserted instructions
uint cut_out_instruction(BytecodeManipulation* manipulation, uint index) {
    uint inserted=0;
    Transformation* transformation=vector_index_transformation(manipulation->transformations, index);
    for(int i=0; i<transformation->inputs_count; i++){
        inserted+=insert_discard(manipulation, transformation->inputs[i], index);
    }
    fill_with_no_op(manipulation, index+inserted, index+inserted);
    return inserted;
}

bool inline_native_calls(BytecodeManipulation* manipulation, vector* instructions, vector* transformations, int pointer, bool print_optimisations) {
    if((vector_index_instruction(instructions, pointer)->type==b_call || vector_index_instruction(instructions, pointer)->type==b_tail_call)
        && vector_index_transformation(transformations, pointer)->inputs[0]->type==d_constant) {
            bool is_tail_call=vector_index_instruction(instructions, pointer)->type==b_tail_call;
            uint arguments_count=vector_index_instruction(instructions, pointer)->uint_argument;
            Transformation* call_transformation=vector_index_transformation(transformations, pointer);
            Object function=call_transformation->inputs[0]->constant_value;
            if(function.type==t_function && function.fp->ftype==f_native){
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
        // search for an instruction that outputs the discarded object
        for(int search=pointer-1; search>=0; search--){
            for(int o=0; o<vector_index_transformation(transformations, search)->outputs_count; o++){
                if(dummies_equal(vector_index_transformation(transformations, search)->outputs[o], discard_input)) {
                    if((vector_index_transformation(transformations, search)->outputs_count==1 || vector_index_instruction(instructions, search)->type==b_double)
                    && !instruction_is_constant(vector_index_instruction(instructions, search), vector_index_transformation(transformations, search))) {
                        LOG_IF_ENABLED("Removing operation which result is immediately discarded:\n")
                        Transformation* producer=vector_index_transformation(transformations, search);
                        // discard inputs to producer
                        int to_discard=producer->inputs_count;
                        if(vector_index_instruction(instructions, search)->type==b_double){
                            to_discard--;
                        }
                        for(int i=0; i<to_discard; i++){
                            Instruction discard_instruction={b_discard};
                            Transformation discard_transformation;
                            transformation_from_instruction(&discard_transformation, &discard_instruction);
                            discard_transformation.inputs[0]=producer->inputs[i];
                            heap_object_reference((HeapObject*)discard_transformation.inputs[0]);
                            if(insert_instruction(manipulation, search, &discard_instruction, &discard_transformation)) {
                                search++;
                                pointer++;
                            }
                        }
                        // remove producer and discard instruction
                        fill_with_no_op(manipulation, search, search);
                        fill_with_no_op(manipulation, pointer, pointer);
                        return true;
                    } else {
                        return false;
                    }
                }
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
    if(vector_index_transformation(transformations, *pointer)->outputs_count==1 
    && vector_index_transformation(transformations, *pointer)->outputs[0]->type==d_constant
    && !instruction_is_literal(vector_index_instruction(instructions, *pointer)->type)
    && instruction_is_constant(vector_index_instruction(instructions, *pointer), vector_index_transformation(transformations, *pointer))){
        Transformation producer=*vector_index_transformation(transformations, *pointer);
        if(constant_dummy_to_bytecode(manipulation->executor, producer.outputs[0], *pointer+1, instructions, transformations, constants)){
            LOG_IF_ENABLED("Replacing operation with it's result:\n")
            // discard inputs to producer
            for(int i=0; i<producer.inputs_count; i++){
                Instruction discard_instruction={b_discard};
                Transformation discard_transformation;
                transformation_from_instruction(&discard_transformation, &discard_instruction);
                discard_transformation.inputs[0]=producer.inputs[i];
                heap_object_reference((HeapObject*)discard_transformation.inputs[0]);
                insert_instruction(manipulation, (*pointer)+1, &discard_instruction, &discard_transformation);
            }
            fill_with_no_op(manipulation, *pointer, *pointer);
            (*pointer)+=producer.inputs_count+1;
            return true;
        }
    }
    return false;
}

bool typed_instructions(vector* instructions, vector* transformations, int pointer){
    #define OPERATOR_VARIANT(base_instruction, type_name) \
    if(vector_index_instruction(instructions, pointer)->type==base_instruction \
        && dummy_type(vector_index_transformation(transformations, pointer)->inputs[0])==(ObjectTypeOrUnknown)t_##type_name \
        && dummy_type(vector_index_transformation(transformations, pointer)->inputs[1])==(ObjectTypeOrUnknown)t_##type_name){ \
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
        && dummy_type(vector_index_transformation(transformations, pointer)->inputs[0])==(ObjectTypeOrUnknown)t_##type_name){ \
            vector_index_instruction(instructions, pointer)->type==base_instruction##_##type_name; \
            return true; \
        }
    OPERATOR_VARIANT(b_minus, int)
    OPERATOR_VARIANT(b_minus, float)
    return false;
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

    BytecodeIterator progress_state;
    for(int p=bytecode_iterator_start(&progress_state, program->code, 0); p!=-1; p=bytecode_iterator_next(&progress_state, program->code)){
        if(p==0){
            for(int i=vector_count(&provided)-1; i>=0; i--){
                vector_push(&stack, vector_index(&provided, i));
            }
        }
        if(!vector_index_transformation(&transformations, p)->visited){
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
                    Dummy* dummy=assumption_to_dummy(E, get_argument_assumption(program, vector_count(&provided)), &dummy_objects_counter);
                    vector_push(&provided, &dummy);
                    transformation.inputs[i]=dummy;
                    // in this case dummy is also referenced by provided
                    heap_object_reference((HeapObject*)transformation.inputs[i]);
                }
                // input is referenced by the transformation
                heap_object_reference((HeapObject*)transformation.inputs[i]);
            }
            predict_instruction_output(E, program, &program->code[p], (char*)program->constants, &dummy_objects_counter, &transformation);
            for(int i=0; i<transformation.outputs_count; i++){
                vector_push(&stack, &transformation.outputs[i]);
                // output is referenced by the transformation
                heap_object_reference((HeapObject*)transformation.outputs[i]);
            }
            *vector_index_transformation(&transformations, p)=transformation;
            vector_index_transformation(&transformations, p)->visited=true;
        } else {
            bool inputs_changed=false;
            for(int i=0; i<vector_index_transformation(&transformations, p)->inputs_count; i++){
                Dummy* from_stack=*(Dummy**)vector_pop(&stack);
                Dummy* expected=(Dummy*)vector_index_transformation(&transformations, p)->inputs[i];
                if(!dummies_equal(expected, from_stack)){
                    if(dummy_contains(from_stack, expected)){
                        heap_object_dereference(E, (HeapObject*)expected);
                        heap_object_reference((HeapObject*)from_stack);
                        vector_index_transformation(&transformations, p)->inputs[i]=from_stack;
                        inputs_changed=true;
                    } else if(!dummy_contains(expected, from_stack)){
                        Dummy* or_dummy=new_or_dummy(E, expected, from_stack, &dummy_objects_counter);
                        // expected is already referenced by the instruction
                        heap_object_reference((HeapObject*)or_dummy);// or_dummy referenced by transformation
                        heap_object_dereference(E, (HeapObject*)expected);// expected is no longer referenced by transformation itself
                        vector_index_transformation(&transformations, p)->inputs[i]=or_dummy;
                        inputs_changed=true;
                    }
                }
            }
            if(inputs_changed){
                // discard all outputs from previous iteration
                for(int i=0; i<vector_index_transformation(&transformations, p)->outputs_count; i++){
                    heap_object_dereference(E, (HeapObject*)vector_index_transformation(&transformations, p)->outputs[i]);
                }
                predict_instruction_output(E, program, &program->code[p], 
                    (char*)program->constants, &dummy_objects_counter, vector_index_transformation(&transformations, p));
                for(int i=0; i<vector_index_transformation(&transformations, p)->outputs_count; i++){
                    vector_push(&stack, &vector_index_transformation(&transformations, p)->outputs[i]);
                    // output is referenced by the transformation
                    heap_object_reference((HeapObject*)vector_index_transformation(&transformations, p)->outputs[i]);
                }
            } else {
                for(int i=0; i<vector_index_transformation(&transformations, p)->outputs_count; i++){
                    Dummy* output=vector_index_transformation(&transformations, p)->outputs[i];
                    vector_push(&stack, &output);
                }
            }
        }
    }
    // fill unreachable transformations with any dummies so their deinitialization doesn't cause errors
    for(int p=0; p<vector_count(&transformations); p++){
        Transformation* transformation=vector_index_transformation(&transformations, p);
        if(!transformation->visited){
            transformation_from_instruction(transformation, vector_index_instruction(&instructions, p));
            for(int i=0; i<transformation->inputs_count; i++){
                transformation->inputs[i]=new_any_dummy(E);
            }
            for(int i=0; i<transformation->outputs_count; i++){
                transformation->outputs[i]=new_any_dummy(E);
            }
        }
    }
    if(print_optimisations){
        printf("Deduced flow chart:\n");
        print_transformations(vector_get_data(&instructions), vector_get_data(&transformations));
        printf("\n");
    }

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

    // remove swap instructions because they are only making code more confusing and prevent some optimisations
    // they will be readded later in the most optimal way
    for(int p=0; p<vector_count(&instructions); p++){
        if(vector_index_instruction(&instructions, p)->type==b_swap){
            fill_with_no_op(&manipulation, p, p);
        }
    }

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
    if(E->options.optimise_stack_operations) {
        stack_usage_optimisations(E, program, &manipulation, &instructions, &transformations, &constants, print_optimisations);
    }
    if(E->options.inline_functions) {
        inline_functions(E, program, &manipulation, &instructions, &informations, &transformations, &constants, print_optimisations);
    }
    if(E->options.optimise_jump_to_return){
        jump_to_return(&manipulation, &instructions, &transformations);
    }
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

    if(print_optimisations){
        printf("Disconnected flow chart:\n");
        print_transformations(vector_get_data(&instructions), vector_get_data(&transformations));
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
        for(int i=0; i<vector_index_transformation(&transformations, p)->inputs_count; i++){
            Dummy* top=*STACK(i);
            Dummy* expected=vector_index_transformation(&transformations, p)->inputs[i];
            if(!dummies_compatible(top, expected)){
                int vector_depth=vector_count(&stack);
                for(int j=1; j<vector_depth; j++){
                    if(dummies_compatible(*STACK(j), expected)){
                        Instruction swap_instruction={b_swap};
                        swap_instruction.swap_argument.left=j;
                        swap_instruction.swap_argument.right=i;
                        Transformation swap_transformation;
                        transformation_from_instruction(&swap_transformation, &swap_instruction);
                        Dummy* any=new_any_dummy(E);
                        for(int k=0; k<swap_transformation.inputs_count; k++){
                            if(k==swap_instruction.swap_argument.left || k==swap_instruction.swap_argument.right){
                                swap_transformation.inputs[k]=*STACK(k);
                                heap_object_reference((HeapObject*)swap_transformation.inputs[k]);
                            } else {
                                swap_transformation.inputs[k]=any;
                                heap_object_reference((HeapObject*)swap_transformation.inputs[k]);
                            }
                        }
                        predict_instruction_output(E, program, &swap_instruction, program->constants, &dummy_objects_counter, &swap_transformation);
                        for(int k=0; k<swap_transformation.outputs_count; k++){
                            heap_object_reference((HeapObject*)swap_transformation.outputs[k]);
                        }
                        Dummy* temp=*STACK(swap_instruction.swap_argument.left);
                        *STACK(swap_instruction.swap_argument.left)=*STACK(swap_instruction.swap_argument.right);
                        *STACK(swap_instruction.swap_argument.right)=temp;
                        vector_insert(&instructions, p, &swap_instruction);
                        vector_insert(&transformations, p, &swap_transformation);
                        vector_insert(&informations, p, vector_index(&informations, p-1));
                        p=bytecode_iterator_next(&progress_state, vector_get_data(&instructions));
                        break;
                    }
                }
            }
        }
        for(int j=0; j<vector_index_transformation(&transformations, p)->inputs_count; j++){
            vector_pop(&stack);
        }
        for(int i=0; i<vector_index_transformation(&transformations, p)->outputs_count; i++){
            vector_push(&stack, &vector_index_transformation(&transformations, p)->outputs[i]);
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
        heap_object_reference((HeapObject*)transformation.inputs[0]);
        vector_insert(&transformations, insert_position, &transformation);
        // take information from end instruction
        vector_push(&informations, vector_top(&informations));
    } */
    
    remove_no_ops(E, &instructions, &informations, &transformations);

    if(print_optimisations){
        printf("\nFinal flow chart:\n");
        print_transformations(vector_get_data(&instructions), vector_get_data(&transformations));
    }

    // cleanup and moving data back from vectors to BytecodeProgram

    while(!vector_empty(&provided)){
        heap_object_dereference(E, (HeapObject*)*(Dummy**)vector_pop(&provided));
    }
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

#undef LOG_IF_ENABLED