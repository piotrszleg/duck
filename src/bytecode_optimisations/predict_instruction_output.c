#include "predict_instruction_output.h"

// function writes to transformation outputs the result of evaluating instruction with it's input
void predict_instruction_output(Executor* E, BytecodeProgram* program, Instruction* instruction, char* constants, unsigned* dummy_objects_counter, Transformation* transformation){
    Dummy** outputs=transformation->outputs;
    Dummy** inputs=transformation->inputs;

    #define OUTPUT_IS_INPUT(output_index, input_index) \
        outputs[output_index]=inputs[input_index]; \
        dummy_reference(outputs[output_index]);
        
    switch (instruction->type){
        case b_null:
            outputs[0]=new_constant_dummy(E, null_const, dummy_objects_counter);
            return;
        case b_jump:
        case b_label:
            for(int i=0; i<transformation->outputs_count; i++){
                OUTPUT_IS_INPUT(transformation->outputs_count-1-i, i)
            }
            return;
        case b_jump_not:
            for(int i=0; i<transformation->outputs_count; i++){
                OUTPUT_IS_INPUT(transformation->outputs_count-1-i, i+1)
            }
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
                dereference(E, &f);
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
            OUTPUT_IS_INPUT(0, 0)
            return;
        case b_double:
            OUTPUT_IS_INPUT(0, 0)
            OUTPUT_IS_INPUT(1, 0)
            return;
        case b_push_to_top:
            OUTPUT_IS_INPUT(instruction->uint_argument, 0)
            OUTPUT_IS_INPUT(0, instruction->uint_argument)
            for(int i=1; i<instruction->uint_argument-1; i++){
                outputs[i]=outputs[instruction->uint_argument-2-i];
            }
            return;
        case b_swap:
        {
            for(int i=0; i<transformation->outputs_count; i++){
                OUTPUT_IS_INPUT(transformation->outputs_count-1-i, i)
            }
            int left=transformation->outputs_count-1-instruction->swap_argument.left;
            int right=transformation->outputs_count-1-instruction->swap_argument.right;
            Dummy* temp=outputs[left];
            OUTPUT_IS_INPUT(left, right)
            outputs[right]=temp;
            dummy_reference(outputs[right]);
            return;
        }
        case b_get:
            if(inputs[0]->type==d_constant){
                outputs[0]=assumption_to_dummy(E, get_upvalue_assumption(program, inputs[0]->constant_value), dummy_objects_counter);
                return;
            }
            break;
        case b_set:
            OUTPUT_IS_INPUT(0, 1)
            return;
        case b_table_set_keep:
            OUTPUT_IS_INPUT(0, 2)
            return;
        case b_binary:
        {
            if(inputs[0]->type==d_constant && inputs[0]->constant_value.type==t_string) {
                ObjectType prediction=operator_predict_result(dummy_type(inputs[1]), dummy_type(inputs[2]), inputs[0]->constant_value.text);
                if(prediction!=t_any){
                    if(inputs[0]->type==d_constant && inputs[1]->type==d_constant && inputs[2]->type==d_constant){
                        Object operator_result=operator(E, inputs[1]->constant_value, inputs[2]->constant_value, inputs[0]->constant_value.text);
                        if(is_unhandled_error(E, operator_result)){
                            error_handle(E, operator_result);
                            dereference(E, &operator_result);
                            outputs[0]=new_any_type_dummy(E, dummy_objects_counter);
                        } else {
                            outputs[0]=new_constant_dummy(E, operator_result, dummy_objects_counter);
                        }
                    } else {
                        outputs[0]=new_known_type_dummy(E, prediction, dummy_objects_counter);
                    }
                    return;
                }
            }
            break;
        }
        #define BINARY(instruction, op) \
            case instruction: \
            { \
                ObjectType prediction=operator_predict_result(dummy_type(inputs[0]), dummy_type(inputs[1]), op); \
                if(prediction!=t_any){ \
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
                        outputs[0]=new_known_type_dummy(E, prediction, dummy_objects_counter); \
                    } \
                    return; \
                } \
                break; \
            }
        #define PREFIX(instruction, op) \
            case instruction: \
            { \
                ObjectType prediction=operator_predict_result(t_null, dummy_type(inputs[0]), op); \
                if(prediction!=t_any){ \
                    if(inputs[0]->type==d_constant) { \
                        outputs[0]=new_constant_dummy(E, operator(E, inputs[0]->constant_value, null_const, op), dummy_objects_counter); \
                    } else { \
                        outputs[0]=new_known_type_dummy(E, prediction, dummy_objects_counter); \
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