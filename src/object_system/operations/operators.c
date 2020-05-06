#include "operators.h"

int compare(Executor* E, Object a, Object b){
    Object error=null_const;
    int comparison_result=compare_and_get_error(E, a, b, &error); \
    if(error.type!=t_null) {
        dereference(E, &error);
    }
    return comparison_result;
}

bool compare_is_constant(ObjectType a, ObjectType b) {
    if(a==t_null||b==t_null){
        return true;
    }
    if((a==t_int||a==t_float)&&(b==t_int||b==t_float)){
        return true;
    }
    if(a!=b){
        return false;
    }
    switch(a) {
        case t_string:
        case t_pointer:
        case t_managed_pointer:
        case t_function:
        case t_symbol:
            return true;
        default:
            return false;
    }
}

// if a>b returns 1 if a<b returns -1, if a==b returns 0
int compare_and_get_error(Executor* E, Object a, Object b, Object* error){
    // null is always less than anything
    if(a.type!=t_null && b.type==t_null){
        return 1;
    }
    if(b.type!=t_null && a.type==t_null){
        return -1;
    }
    if(b.type==t_null && a.type==t_null){
        return 0;
    }
    if(a.type==t_float && b.type==t_int){
        return sign(a.float_value-b.int_value);
    }
    if(a.type==t_int && b.type==t_float){
        return sign((float)a.int_value-b.float_value);
    }
    if(a.type!=b.type){
        return 1;// objects of different types can't be equal
    }
    switch(a.type){
        case t_string:
            return strcmp(a.text, b.text);
        case t_int:
        case t_copy_replace_promise:
            return sign(a.int_value-b.int_value);
        case t_float:
            return sign(a.float_value-b.float_value);
        case t_table: {
            Object compare_override=get(E, a, OVERRIDE(E, compare));
            if(compare_override.type!=t_null){
                Object call_result=call(E, compare_override, (Object[]){a, b}, 2);
                dereference(E, &compare_override);
                if(call_result.type==t_int){
                    return call_result.int_value;
                } else {
                    NEW_ERROR(*error, "COMPARISON_ERROR", multiple_causes(E, (Object[]){a, b, call_result}, 3), "Function at field compare didn't return an int value.");
                    return 1;
                }
            } else {
                dereference(E, &compare_override);
                return table_compare(E, a.tp, b.tp, error);
            }
        }
        case t_pointer:
            return sign((long unsigned)a.p-(long unsigned)b.p);
        case t_managed_pointer:
        case t_coroutine:
            return sign(a.hp-b.hp);
        case t_symbol:
            return sign(a.sp->index-b.sp->index);
        case t_function:
            if(a.fp->ftype!=b.fp->ftype){
                return 1;
            }
            switch(a.fp->ftype){
                case f_ast:
                case f_bytecode:
                    return sign((long)a.fp->source_pointer-(long)b.fp->source_pointer);
                case f_special:
                    return sign((long)a.fp->special_index-(long)b.fp->special_index);
                case f_native:
                    return sign((long)a.fp->native_pointer-(long)b.fp->native_pointer);
            }
        default:
            NEW_ERROR(*error, "COMPARISON_ERROR", multiple_causes(E, (Object[]){a, b}, 2), "Can't compare objects of type %s", get_type_name(a.type));
            return 1;
    }
}

ObjectType operator_predict_result(ObjectType a, ObjectType b, const char* op) {
    if(a==t_table || a==t_any || b==t_any){
        return t_any;
    }
    size_t op_length=strlen(op);
    if(op_length==1) {
        switch(op[0]){
            case '!': return t_int;
            case '#': return t_any;
        }
        #define CASE(character) \
            case character: \
                if(cast_is_constant(b, a)){ \
                    return a; \
                } else { \
                    return t_any; \
                }
        if(a==t_int) {
            switch(op[0]){
                CASE('+')
                CASE('-')
                CASE('*')
                CASE('%')
                CASE('/')
            }
        }
        // '-' prefix
        if(a==t_null && op[0]=='-'&&(b==t_int||b==t_float)) {
            return b;
        }
        if(a==t_float) {
            switch(op[0]){
                CASE('+')
                CASE('-')
                CASE('*')
                CASE('/')
            }
        }
        #undef CASE
    } else {  
        if(strcmp(op, "--")==0) return t_any;
        if(strcmp(op, "><")==0) return t_any;
        if(strcmp(op, "##")==0) return t_any;
        if(strcmp(op, "&&")==0) return t_int;
        if(strcmp(op, "||")==0) return t_int;
        if(strcmp(op, "//")==0) { 
            if(a==t_int && cast_is_constant(b, a)) {
                return t_int;
            } else{
                return t_any;
            }
        }
        #define COMPARISSON(operator_name) \
            if(strcmp(op, operator_name)==0) { \
                if(compare_is_constant(a, b)) { \
                    return t_int; \
                } else { \
                    return t_any; \
                } \
            }
        COMPARISSON("==") 
        COMPARISSON("!=")
        COMPARISSON(">")
        COMPARISSON("<")
        COMPARISSON(">=")
        COMPARISSON("<=")
        COMPARISSON("is")
        COMPARISSON("compare")
        #undef COMPARISSON
    }
    return t_any;
}

bool is(Executor* E, Object a, Object b){
    if(a.type!=b.type){
        return false;
    } else if(is_heap_object(a)) {
        return a.hp==b.hp;
    } else if(a.type==t_string) {
        return a.text==b.text;
    } else {
        return compare(E, a, b)==0;
    }
}

Object operator(Executor* E, Object a, Object b, const char* op){
    size_t op_length=strlen(op);
    if(a.type==t_table){
        Object operator_override=get(E, a, OVERRIDE(E, operator));
        if(operator_override.type!=t_null){
            // call get_function a and b as arguments
            Object result=call(E, operator_override, OBJECTS_ARRAY(a, b, to_string(op)), 3);
            dereference(E, &operator_override);
            return result;
        }
        dereference(E, &operator_override);
    }
    #define OP_CASE(operator_name) if(strcmp(op, operator_name)==0)
    #define COMPARISON_OPERATOR(check) { \
        Object error=null_const; \
        int comparison_result=compare_and_get_error(E, a, b, &error); \
        if(error.type!=t_null){ \
            return error; \
        } else { \
            return to_int(check); \
        } \
    }
    #define COMPARISON_OPERATOR_CASE(operator_name, check) \
        OP_CASE(operator_name) { \
            COMPARISON_OPERATOR(check) \
        }
    if(op_length==1){
        // call b with iterator result's value for each iteration
        switch(op[0]){
            case '#': {
                Object it;
                Object call_result=null_const;
                FOREACH(a, it, 
                    call_result=call(E, b, OBJECTS_ARRAY(get(E, it, to_string("value"))), 1);
                )
                return call_result;
            }
            case '!':
                return to_int(is_falsy(b));
            case '-':
                if(a.type==t_null && b.type==t_int){
                    return to_int(-b.int_value);
                } else if(a.type==t_null && b.type==t_float){
                    return to_float(-b.float_value);
                } else {
                    break;
                }
            case '>':
                COMPARISON_OPERATOR(comparison_result==1)
            case '<':
                COMPARISON_OPERATOR(comparison_result==-1)
        }
    } else if(op_length==2){
        COMPARISON_OPERATOR_CASE("==", comparison_result==0)
        COMPARISON_OPERATOR_CASE("!=", comparison_result!=0)
        COMPARISON_OPERATOR_CASE(">=", comparison_result==1||comparison_result==0)
        COMPARISON_OPERATOR_CASE("<=", comparison_result==-1||comparison_result==0)
        OP_CASE("is"){
            return to_int(is(E, a, b));
        }
        OP_CASE("||"){
            if(is_truthy(a)){
                return a;
            } else if (is_truthy(b)){
                return b;
            } else {
                Object result={t_null};
                return result;
            }
        }
        OP_CASE("&&"){
            if(is_falsy(a)){
                return a;
            } else if (is_falsy(b)){
                return b;
            } else{
                return b;
            }
        }
        OP_CASE("--"){
            return to_pipe(E, a, b);
        }
        OP_CASE("><"){
            return to_binding(E, a, b);
        }
        // call b with arguments key and value for each iteration
        OP_CASE("##"){
            Object it;
            Object call_result=null_const;
            FOREACH(a, it, 
                call_result=call(E, b, OBJECTS_ARRAY(get(E, it, to_string("key")), get(E, it, to_string("value"))), 2);
            )
            return call_result;
        }
    } else {
        COMPARISON_OPERATOR_CASE("compare", comparison_result)
    }
    switch(a.type){
        case t_string:
            if(op_length!=1){
                break;
            }
            switch(op[0]){
                case '+': {
                    Object result;
                    if(b.type!=t_string) {
                        USING_STRING(stringify(E, b),
                            result=to_string(string_add(a.text, str)))
                    } else {
                        result=to_string(string_add(a.text, b.text));
                    }
                    return result;
                }
                case '*':
                    if(b.type==t_int && b.int_value>0){
                        return to_string(string_repeat(a.text, b.int_value));
                    }
                    break;
            }
            break;

        #define NUMERICAL_OPERATOR(result) \
            { \
                bool b_was_casted=false; \
                if(b.type!=a.type){/* checking it here avoids unecessary function call */ \
                    b=cast(E, b, a.type); \
                    b_was_casted=true; \
                    if(b.type!=a.type){ \
                        return b;/* b is conversion error */ \
                    } \
                } \
                Object _result=result; \
                if(b_was_casted){ \
                    dereference(E, &b); \
                } \
                return _result; \
            }
        #define NUMERICAL_OPERATOR_CASE(character, result) \
            case character: \
                NUMERICAL_OPERATOR(result)
        case t_int:{
            if(op_length==2){
                if(op[0]=='/' && op[1]=='/'){
                    NUMERICAL_OPERATOR(to_int(a.int_value/b.int_value))
                }
            } else if(op_length==1){
                switch(op[0]){
                    NUMERICAL_OPERATOR_CASE('+', to_int(a.int_value+b.int_value))
                    NUMERICAL_OPERATOR_CASE('-', to_int(a.int_value-b.int_value))
                    NUMERICAL_OPERATOR_CASE('*', to_int(a.int_value*b.int_value))
                    NUMERICAL_OPERATOR_CASE('%', to_int(a.int_value%b.int_value))
                    NUMERICAL_OPERATOR_CASE('/', to_float((float)a.int_value/b.int_value))
                }
            }
            break;
        }
        case t_float: {
            if(op_length==1){
                switch(op[0]){
                    NUMERICAL_OPERATOR_CASE('+', to_float(a.float_value+b.float_value))
                    NUMERICAL_OPERATOR_CASE('-', to_float(a.float_value-b.float_value))
                    NUMERICAL_OPERATOR_CASE('*', to_float(a.float_value*b.float_value))
                    NUMERICAL_OPERATOR_CASE('/', to_float(a.float_value/b.float_value))
                }
            }
            break;
        }
        default:;
    }
    PATCH(operator, a.type, a, b);
    RETURN_ERROR("OPERATOR_ERROR", multiple_causes(E, OBJECTS_ARRAY(a, b), 2), "Can't perform operotion '%s' on objects of type <%s> and <%s>", op, get_type_name(a.type), get_type_name(b.type));
}