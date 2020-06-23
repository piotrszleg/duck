#include "basic.h"

bool is_truthy(Object o){
    return !is_falsy(o);
}

bool is_falsy(Object o){
    switch(o.type){
        case t_null:
            return 1;// null is falsy
        case t_string:
            return strlen(o.text)==0;// "" (string of length 0) is falsy
        case t_int:
            return o.int_value==0;// 0 is falsy
        case t_float:
            return o.float_value==0;// 0 is falsy
        case t_table:
            return o.tp->elements_count==0;// empty table is falsy
        case t_function:
        case t_managed_pointer:
        case t_coroutine:
        case t_pointer:
            return false;
        default:
            INCORRECT_ENUM_VALUE(ObjectType, o, o.type);
            return false;
    }
}

bool cast_is_constant(ObjectType from, ObjectType to) {
    if(from==to) {
        return true;
    }
    switch(to) {
        case t_string: return true;
        case t_int:
            return from==t_float||from==t_null;
        case t_float:
            return from==t_int||from==t_null;
        default:
            return false;
    }
}

static bool is_int_literal(const char *s) {
    while (*s!='\0') {
        if (!isdigit(*s)){
            return false;
        } else {
            s++;
        }
    }
    return true;
}

static bool is_float_literal(const char *s) {
    while (*s!='\0') {
        if (!(isdigit(*s) || *s=='.')){
            return false;
        } else {
            s++;
        }
    }
    return true;
}

Object cast(Executor* E, Object o, ObjectType type){
    if(o.type==type){
        reference(&o);
        return o;
    }
    if(o.type==t_table){
        Object cast_override=get_ignore_topmost_prototypes(E, o, OVERRIDE(E, cast));
        if(cast_override.type!=t_null){
            // call get_function a and b as arguments
            Object result=call(E, cast_override, (Object[]){o, to_string(get_type_name(type))}, 2);
            dereference(E, &cast_override);
            // if function returned null continue searching for casting method
            if(result.type!=t_null){
                return result;
            }
        }
    }
    switch(type){
        case t_string:
        {
            return to_string(stringify(E, o));
        }
        case t_int:
        {
            if(o.type==t_float){
                return to_int(o.float_value);
            } else if(o.type==t_null){
                return to_int(0);
            } else if(o.type==t_string && is_int_literal(o.text)){
                return to_int(strtol(o.text, NULL, 10));// convert string to int if it contains number
            }
            break;
        }
        case t_float:
        {
            if(o.type==t_int){
                return to_float(o.int_value);
            }else if(o.type==t_null){
                return to_float(0.0f);
            } else if(o.type==t_string && is_float_literal(o.text)){
                return to_float(strtof(o.text, NULL));// convert string to float if it contains number
            }
            break;
        }
        default:;
    }
    PATCH(cast, o.type, o, to_string(get_type_name(type)))
    RETURN_ERROR("TYPE_CONVERSION_FAILURE", o, "Can't convert from <%s> to <%s>", get_type_name(o.type), get_type_name(type));
}

static unsigned hash_string(const char *str) {
    unsigned hashed = 5381;
    while (*str) {
        hashed = ((hashed << 5) + hashed) ^ *str++;
    }
    return hashed;
}

unsigned hash_and_get_error(Executor* E, Object o, Object* error) {
    switch(o.type){
        case t_string:
            return hash_string(o.text);
        case t_int:
        case t_copy_replace_promise:
            return o.int_value;
        case t_float:
            return o.float_value;
        case t_null:
            return 0;
        case t_table: {
            Object hash_override=get_ignore_topmost_prototypes(E, o, OVERRIDE(E, hash));
            if(hash_override.type!=t_null){
                Object call_result=call(E, hash_override, &o, 1);
                dereference(E, &hash_override);
                if(call_result.type!=t_int){
                    NEW_ERROR(*error, "HASH_ERROR", multiple_causes(E, (Object[]){o, call_result}, 2), "Function at field hash didn't return an int value.");
                    return 0;
                }
                // transform int bits to uint bits
                union {
                    int int_value;
                    uint uint_value;
                } hash={call_result.int_value};
                return hash.uint_value;
            } else {
                dereference(E, &hash_override);
                return table_hash(E, o.tp, error);
            }
        }
        case t_function:
            switch(o.fp->ftype){
                case f_ast:
                case f_bytecode:
                    return (unsigned)(long unsigned)o.fp->source_pointer;
                case f_special:
                    return (unsigned)(long unsigned)o.fp->special_index;
                case f_native:
                    return (unsigned)(long unsigned)o.fp->native_pointer;
            }
         case t_pointer:
            return (unsigned)(long unsigned)o.p;
        case t_managed_pointer:
            return (unsigned)(long unsigned)o.mp;
        case t_symbol:
            return o.sp->index;
        default:
            NEW_ERROR(*error, "HASH_ERROR", o, "Can't hash object of type <%s>", get_type_name(o.type));
            return 0;
    }
}

uint hash(Executor* E, Object key){
    Object error=null_const;
    uint hashed=hash_and_get_error(E, key, &error);
    if(error.type!=t_null){
        dereference(E, &error);
    }
    return hashed;
}

// result is not referenced, only viewed
#define GET_OR_RETURN_ERROR(result, key) \
{ \
    Object error=null_const; \
    result=table_get_with_hashing_error(E, o.tp, key, &error); \
    if(error.type!=t_null){ \
        return error; \
    } \
}

Object get_prototype_override(Executor* E, Object o){
    if(o.type==t_table){
        Object result;
        GET_OR_RETURN_ERROR(result, OVERRIDE(E, prototype))
        if(result.type!=t_null){
            reference(&result);
            return result;
        }
    }
    return null_const;
}

Object get_prototype(Executor* E, Object o){
    Object result=get_prototype_override(E, o);
    if(result.type!=t_null){
        return result;
    }
    return OBJECT_SYSTEM(E)->types_objects[o.type];
}

Object get_common(Executor* E, Object o, Object key){
    switch(o.type){
        case t_table:
        {
            // try to get "get" field overriding function from the table and use it
            Object get_override;
            GET_OR_RETURN_ERROR(get_override, OVERRIDE(E, get))
            if(get_override.type!=t_null){
                Object arguments[]={o, key};
                return call(E, get_override, arguments, 2);
            } else {
                Object result;
                GET_OR_RETURN_ERROR(result, key)
                if(result.type!=t_null) {
                    reference(&result);
                    return result;
                }
            }
            break;
        }
        #define FIELD(identifier) if(key.type==t_string && strcmp(key.text, identifier)==0)
        case t_string:
            FIELD("length"){
                return to_int(strlen(o.text));
            } else if(key.type==t_int){
                if(key.int_value<strlen(o.text) && key.int_value>=0){
                    char* character_string=malloc(2*sizeof(char));
                    character_string[0]=o.text[(int)key.int_value];
                    character_string[1]='\0';
                    return to_string(character_string);
                } else {
                    RETURN_ERROR("GET_ERROR", multiple_causes(E, (Object[]){o, key}, 2), 
                    "Index %i is out of bounds of string \"%s\"", key.int_value, o.text);
                }
            }
            break;
        case t_coroutine:
            FIELD("state"){
                switch(o.co->state){
                    case co_uninitialized: return to_string(strdup("uninitialized"));
                    case co_running: return to_string(strdup("running"));
                    case co_finished: return to_string(strdup("finished"));
                }
            }
            FIELD("finished"){
                return to_int(o.co->state==co_finished);
            }
            break;
        case t_function:
            FIELD("variadic"){
                return to_int(o.fp->variadic);
            }
            FIELD("arguments_count"){
                return to_int(o.fp->arguments_count);
            }
            break;
        case t_symbol:
            FIELD("comment"){
                return to_string(strdup(o.sp->comment));
            }
            break;
        default:;
    }
    return null_const;
}

Object get_ignore_topmost_prototypes(Executor* E, Object o, Object key){
    Object result=get_common(E, o, key);
    if(result.type!=t_null){
        return result;
    }
    Object prototype=get_prototype_override(E, o);
    if(prototype.type!=t_null){
        Object result=get_ignore_topmost_prototypes(E, prototype, key);
        dereference(E, &prototype);
        if(result.type!=t_null){
            return result;
        }
    }
    return null_const;
}
    

Object get(Executor* E, Object o, Object key){
    Object result=get_common(E, o, key);
    if(result.type!=t_null){
        return result;
    }
    Object prototype=get_prototype(E, o);
    if(prototype.type!=t_null){
        Object result=get_ignore_topmost_prototypes(E, prototype, key);
        if(result.type!=t_null){
            return result;
        }
    }
    PATCH(get, o.type, o, key);
    return null_const;
}

Object set(Executor* E, Object o, Object key, Object value){
    if(o.type==t_table){
        // try to get "get" operator overriding function from the Table and use it
        Object set_override=get_ignore_topmost_prototypes(E, o, OVERRIDE(E, set));
        if(set_override.type!=t_null){
            Object result=call(E, set_override, (Object[]){o, key, value}, 3);
            dereference(E, &set_override);
            return result;
        } else if(table_is_protected(o.tp)){
            RETURN_ERROR("SET_ERROR", multiple_causes(E, (Object[]){o, key, value}, 3), 
                         "Attempted to set a field in a protected table.");
        } else {
            table_set(E, o.tp, key, value);
            reference(&value);
            return value;
        }
    } else {
        PATCH(set, o.type, o, key);
        RETURN_ERROR("SET_ERROR", o, "Can't set field in object of type <%s>", get_type_name(o.type));
    }
}

Object call(Executor* E, Object o, Object* arguments, int arguments_count) {
    switch(o.type){
        case t_function:
        {
            return call_function(E, o.fp, arguments, arguments_count);
        }
        case t_coroutine:
        {
            return call_coroutine(E, o.co, arguments, arguments_count);
        }
        case t_table:
        {
            Object call_override=get_ignore_topmost_prototypes(E, o, OVERRIDE(E, call));
            if(call_override.type!=t_null){
                // add o object as a first argument
                Object* arguments_with_self=malloc(sizeof(Object)*(arguments_count+1));
                arguments_with_self[0]=o;
                memcpy(arguments_with_self+1, arguments, arguments_count*sizeof(Object));

                Object result=call(E, call_override, arguments_with_self, arguments_count+1);
                free(arguments_with_self);
                return result;
            }// else go to default label
        }
        default: {
            Object patch=get_patch(E, o.type, OVERRIDE(E, call));
            if(patch.type!=t_null){
                Object* arguments_with_self=malloc(sizeof(Object)*(arguments_count+1));
                arguments_with_self[0]=o;
                memcpy(arguments_with_self+1, arguments, arguments_count*sizeof(Object));
                Object result=call(E, patch, arguments_with_self, arguments_count+1);
                free(arguments_with_self);
                dereference(E, &patch);
                return result;
            } else {
                RETURN_ERROR("WRONG_ARGUMENT_TYPE", o, "Can't call object of type <%s>", get_type_name(o.type));
            }
        }
    }
}

void call_destroy(Executor* E, Object o){
    Object destroy_override=get_ignore_topmost_prototypes(E, o, OVERRIDE(E, destroy));
    if(destroy_override.type!=t_null){
        Object destroy_result=call(E, destroy_override, &o, 1);
        dereference(E, &destroy_override);
        dereference(E, &destroy_result);
    }
}

#undef ALREADY_DESTROYED_CHECK