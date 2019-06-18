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

typedef enum {
    d_any,
    d_any_type,
    d_known_type,
    d_constant
} DummyType;

typedef struct
{
    gc_Pointer gcp;
    unsigned int id;
    DummyType type;
    union {
        ObjectType known_type;
        Object constant_value;
    };
} Dummy;

void dummy_destructor(Executor* E, Dummy* dummy){
    free(dummy);
}

Dummy* new_dummy(Executor* E){
    Dummy* result=malloc(sizeof(Dummy));
    gc_pointer_init(E, (gc_Pointer*)result, (gc_PointerDestructorFunction)dummy_destructor);
    return result;
}

typedef struct
{
    Dummy** inputs;
    Dummy** outputs;
} Transformation;

ObjectType dummy_type(const Dummy* dummy){
    if(dummy->type==d_known_type){
        return dummy->known_type;
    } else if(dummy->type==d_constant){
        return dummy->constant_value.type;
    } else {
        return t_null;
    }
}

// function writes to outputs
// length of outputs array is deduced from instruction
void predict_instruction_output(Executor* E, Instruction* instr, char* constants, unsigned int* id_counter, Dummy** inputs, Dummy** outputs){
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
        {
            outputs[0]=inputs[0];
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
            for(int i=0; i<pushes_to_stack(*instr); i++){
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
        default:
            THROW_ERROR(BYTECODE_ERROR, "Incorrect dummy type %i.", dummy->type);
    }
}

static void print_transformations(Instruction* instructions, Transformation* transformations, int instructions_count){
    printf("Instructions transformations:\n");
    for(int p=0; p<instructions_count; p++){
        printf("%s (", INSTRUCTION_NAMES[instructions[p].type]);
        for(int i=0; i<gets_from_stack(instructions[p]); i++){
            if(i!=0){
                printf(", ");
            }
            dummy_print(transformations[p].inputs[i]);
        }
        printf(")");
        printf("->(");
        for(int i=0; i<pushes_to_stack(instructions[p]); i++){
            if(i!=0){
                printf(", ");
            }
            dummy_print(transformations[p].outputs[i]);
        }
        printf(")\n");
    }
}

void replace_dummy(Executor* E, Instruction* instructions, Transformation* transformations, int instructions_count, Dummy* to_replace, Dummy* replacement){
    for(int p=0; p<instructions_count; p++){
        for(int i=0; i<gets_from_stack(instructions[p]); i++){
            if(dummies_equal(to_replace, transformations[p].inputs[i])){
                gc_object_dereference(E, (gc_Object*)transformations[p].inputs[i]);
                gc_object_reference((gc_Object*)replacement);
                transformations[p].inputs[i]=replacement;
            }
        }
        for(int i=0; i<pushes_to_stack(instructions[p]); i++){
            if(dummies_equal(to_replace, transformations[p].outputs[i])){
                 gc_object_dereference(E, (gc_Object*)transformations[p].outputs[i]);
                 gc_object_reference((gc_Object*)replacement);
                transformations[p].outputs[i]=replacement;
            }
        }
    }
}

void transformation_init(Transformation* transformation, Instruction* instruction){
    int gets=gets_from_stack(*instruction);
    transformation->inputs=malloc(sizeof(Dummy*)*gets);
    if(gets){
        CHECK_ALLOCATION(transformation->inputs)
    }
    int pushes=pushes_to_stack(*instruction);
    transformation->outputs=malloc(sizeof(Dummy*)*pushes);
    if(pushes){
        CHECK_ALLOCATION(transformation->outputs)
    }
}

void transformation_deinit(Executor* E, Transformation* t, Instruction* instr){
    for(int i=0; i<gets_from_stack(*instr); i++){
        gc_object_dereference(E, (gc_Object*)t->inputs[i]);
    }
    for(int i=0; i<pushes_to_stack(*instr); i++){
        gc_object_dereference(E, (gc_Object*)t->outputs[i]);
    }
    free(t->inputs);
    t->inputs=NULL;
    free(t->outputs);
    t->outputs=NULL;
}

void remove_no_ops(Executor* E, vector* instructions, vector* informations, vector* transformations){
    #define INSTRUCTION(nth) ((Instruction*)vector_index(instructions, (nth)))
    int block_start=0;
    bool inside_block=false;
    for(int p=0; p<vector_count(instructions); p++){
        if(inside_block){
            if(INSTRUCTION(p)->type!=b_no_op || INSTRUCTION(p)->type==b_end){
                for(int i=block_start; i<=p-1; i++){
                    transformation_deinit(E, (Transformation*)vector_index(transformations, i), INSTRUCTION(i));
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

void optimise_bytecode(Executor* E, BytecodeProgram* prog, bool print_optimisations){
    for(int i=0; i<prog->sub_programs_count; i++){
        optimise_bytecode(E, &prog->sub_programs[i], print_optimisations);
    }
    if(print_optimisations){
        USING_STRING(stringify_bytecode(prog),
            printf("Unoptimised bytecode:\n%s\n", str));
    }
    int instructions_count=count_instructions(prog->code)+1;
    vector provided, dummy_stack, transformations;
    vector_init(&provided, sizeof(Dummy*), 64);
    vector_init(&dummy_stack, sizeof(Dummy*), 128);
    vector_init(&transformations, sizeof(Transformation), instructions_count);
    unsigned int dummy_objects_counter=0;
    for(int p=0; p<instructions_count; p++){
        Transformation transformation;
        transformation_init(&transformation, &prog->code[p]);
        for(int i=0; i<gets_from_stack(prog->code[p]); i++){
            if(!vector_empty(&dummy_stack)){
                transformation.inputs[i]=*(Dummy**)vector_pop(&dummy_stack);
            } else {
                Dummy* dummy=new_dummy(E);
                dummy->id=dummy_objects_counter++;
                dummy->type=d_any_type;
                vector_push(&provided, &dummy);
                gc_object_reference((gc_Object*)dummy);// in this case dummy is also refrenced by provided
                transformation.inputs[i]=dummy;
            }
            gc_object_reference((gc_Object*)transformation.inputs[i]);
        }
        predict_instruction_output(E, &prog->code[p], (char*)prog->constants, &dummy_objects_counter, transformation.inputs, transformation.outputs);
        for(int i=0; i<pushes_to_stack(prog->code[p]); i++){
            vector_push(&dummy_stack, &transformation.outputs[i]);
            // output is refrenced by stack and instruction
            gc_object_reference((gc_Object*)transformation.outputs[i]);
            gc_object_reference((gc_Object*)transformation.outputs[i]);
        }
        vector_push(&transformations, &transformation);
    }

    print_transformations(prog->code, vector_get_data(&transformations), instructions_count);

    vector instructions, informations;
    vector_from(&instructions, sizeof(Instruction), prog->code, instructions_count);
    vector_from(&informations, sizeof(InstructionInformation), prog->information, instructions_count);

    #define INSTRUCTION(nth) ((Instruction*)vector_index(&instructions, (nth)))
    #define TRANSFORMATION(nth) ((Transformation*)vector_index(&transformations, (nth)))
    #define CODE ((Instruction*)vector_get_data(&instructions))
    #define FILL_WITH_NO_OP(start, end) \
        for(int i=start; i<=end; i++) { \
            transformation_deinit(E, TRANSFORMATION(i), INSTRUCTION(i)); \
            INSTRUCTION(i)->type=b_no_op; \
        }
    for(int pointer=instructions_count-1; pointer>=0; pointer--){
        if(INSTRUCTION(pointer)->type==b_call){
            if(INSTRUCTION(pointer+1)->type==b_return || INSTRUCTION(pointer+1)->type==b_end){
                INSTRUCTION(pointer)->type=b_tail_call;
            }
        } else if(prog->code[pointer].type==b_set && prog->code[pointer+1].type==b_discard
           && !prog->code[pointer].bool_argument /* argument tells whether the variable is used in closure, we can't tell if the closure changes the variable*/
           && path_length(prog->code, pointer)<=2){// don't optimise nested paths like table.key, only single name paths
            if(print_optimisations){
                printf("Found a set Instruction\n");
                prog->code=vector_get_data(&instructions);
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
                        prog->code=vector_get_data(&instructions);
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
                        transformation_init(&double_transformation, &double_instruction);
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
                    replace_dummy(E, (Instruction*)vector_get_data(&instructions), (Transformation*)vector_get_data(&transformations), vector_count(&instructions),
                    TRANSFORMATION(search_pointer)->outputs[0], 
                    TRANSFORMATION(pointer)->outputs[0]);
                    //search_pointer+=keep_vector_top_unchanged(prog, pointer+1, search_pointer-get_path_length, print_optimisations);
                    FILL_WITH_NO_OP(search_pointer-get_path_length+1, search_pointer);

                    if(search_pointer>=vector_count(&instructions)){
                        break;// after removing the path search_pointer points to b_end, if loop continued from this point it would get past the code's end
                    }
                }
            }
            if(!used){
                // the variable isn't used in it's own scope and in any closure, so it can be removed
                LOG_IF_ENABLED("Removing set Instruction:\n");
                replace_dummy(E, (Instruction*)vector_get_data(&instructions), (Transformation*)vector_get_data(&transformations), vector_count(&instructions),
                TRANSFORMATION(pointer)->outputs[0], 
                TRANSFORMATION(pointer)->inputs[1]);
                FILL_WITH_NO_OP(pointer-path_length(CODE, pointer)+1, pointer);
            }
        }
    }
    while(!vector_empty(&dummy_stack)){
        gc_object_dereference(E, (gc_Object*)*(Dummy**)vector_pop(&dummy_stack));
    }
    while(!vector_empty(&provided)){
        vector_push(&dummy_stack, vector_pop(&provided));
    }
    #define STACK(nth) ((Dummy**)vector_index(&dummy_stack, vector_count(&dummy_stack)-1-(nth)))
    for(int p=0; p<vector_count(&instructions); p++){
        for(int i=0; i<gets_from_stack(*INSTRUCTION(p)); i++){
            Dummy* top=*STACK(i);
            Dummy* expected=TRANSFORMATION(p)->inputs[i];
            if(!dummies_equal(top, expected)){
                int vector_depth=vector_count(&dummy_stack);
                for(int j=1; j<vector_depth; j++){
                    if(dummies_equal(*STACK(j), expected)){
                        Instruction swap_instruction={b_swap};
                        swap_instruction.swap_argument.left=j;
                        swap_instruction.swap_argument.right=i;
                        Transformation swap_transformation;
                        transformation_init(&swap_transformation, &swap_instruction);
                        Dummy* any=new_dummy(E);
                        any->type=d_any;
                        for(int k=0; k<j+1; k++){
                            if(k!=swap_instruction.swap_argument.left && k!=swap_instruction.swap_argument.right){
                                swap_transformation.inputs[k]=swap_transformation.outputs[k]=any;
                                gc_object_reference((gc_Object*)swap_transformation.inputs[k]);
                                gc_object_reference((gc_Object*)swap_transformation.outputs[k]);
                            }
                        }
                        swap_transformation.inputs[swap_instruction.swap_argument.left]  =*STACK(swap_instruction.swap_argument.left);
                        swap_transformation.inputs[swap_instruction.swap_argument.right] =*STACK(swap_instruction.swap_argument.right);
                        swap_transformation.outputs[swap_instruction.swap_argument.left] =*STACK(swap_instruction.swap_argument.right);
                        swap_transformation.outputs[swap_instruction.swap_argument.right]=*STACK(swap_instruction.swap_argument.left);
                        gc_object_reference((gc_Object*)swap_transformation.inputs[swap_instruction.swap_argument.left]  );
                        gc_object_reference((gc_Object*)swap_transformation.inputs[swap_instruction.swap_argument.right] );
                        gc_object_reference((gc_Object*)swap_transformation.outputs[swap_instruction.swap_argument.left] );
                        gc_object_reference((gc_Object*)swap_transformation.outputs[swap_instruction.swap_argument.right]);
                        Dummy* temp=*STACK(swap_instruction.swap_argument.left);
                        *STACK(swap_instruction.swap_argument.left)=*STACK(swap_instruction.swap_argument.right);
                        *STACK(swap_instruction.swap_argument.right)=temp;
                        vector_insert(&instructions, p, &swap_instruction);
                        vector_insert(&transformations, p, &swap_transformation);
                        vector_insert(&informations, p, vector_index(&informations, p));
                        p++;
                        instructions_count++;
                        break;
                    }
                }
            }
        }
        for(int j=0; j<gets_from_stack(*INSTRUCTION(p)); j++){
            gc_object_dereference(E, (gc_Object*)*(Dummy**)vector_pop(&dummy_stack));
        }
        for(int i=0; i<pushes_to_stack(*INSTRUCTION(p)); i++){
            vector_push(&dummy_stack, &TRANSFORMATION(p)->outputs[i]);
            gc_object_reference((gc_Object*)TRANSFORMATION(p)->outputs[i]);
        }
    }
    remove_no_ops(E, &instructions, &informations, &transformations);
    prog->code=vector_get_data(&instructions);
    prog->information=vector_get_data(&informations);

    printf("\n");
    print_transformations(prog->code, vector_get_data(&transformations), vector_count(&instructions));

    vector_deinit(&provided);
    vector_deinit(&dummy_stack);
    for(int i=0; i<vector_count(&transformations); i++){
        transformation_deinit(E, TRANSFORMATION(i), INSTRUCTION(i));
    }
    vector_deinit(&transformations);

    #undef FILL_WITH_NO_OP
    #undef INSTRUCTION
    #undef TRANSFORMATION
    #undef CODE
}

#undef LOG