#include "struct_descriptor.h"

#define GET_INT(result_name, exp) \
    { \
        Object o=exp;\
        if(o.type!=t_int) { \
            RETURN_ERROR("INCORRECT_ARGUMENT_TYPE", o, "Field type should be number"); \
        } else { \
            result_name=(int)o.int_value; \
        } \
    }

char* NATIVE_TYPES_NAMES[]={
    #define X(t) #t,
    INT_TYPES
    #undef X
    "float",
    "double",
    "long double",
    "string",
    "struct",
    "pointer"
};

static Object struct_descriptor_set(Executor* E, Object scope, Object* arguments, int arguments_count);
static Object struct_descriptor_get(Executor* E, Object scope, Object* arguments, int arguments_count);

bool is_struct_descriptor(Executor* E, Object o){
    if(o.type!=t_table){
        return false;
    }
    Object is_struct_descriptor=table_get(E, o.tp, to_string("is_struct_descriptor"));
    bool result=is_truthy(is_struct_descriptor);
    dereference(E, &is_struct_descriptor);
    return result;
}

void add_struct_descriptor_fields(Executor* E, Table* sd, void* position);

Object field_get(Executor* E, void* position, Object field){
    int type;
    REQUIRE_TYPE(field, t_table)
    GET_INT(type, table_get(E, field.tp, to_string("type")));

    switch(type){
        #define X(t) \
        case n_##t: \
            return to_int(*(t*)position);
        INT_TYPES
        #undef X
        case n_float:
            return to_float(*(float*)position);
        case n_string:
            return to_string(*(char**)position);
        case n_struct:
            return new_struct_descriptor(E, position, table_get(E, field.tp, to_string("fields")));
        case n_pointer:
        {
            void* pointed_position=*(void**)position;
            Object pointed=table_get(E, field.tp, to_string("pointed"));
            REQUIRE_TYPE(pointed, t_table)
            add_struct_descriptor_fields(E, pointed.tp, pointed_position);
            return pointed;
        }
        default: RETURN_ERROR("STRUCTURE_GET_ERROR", field, "Field has incorrect type value %i.", type);
    }
}

Object struct_get_field(Executor* E, void* position, Object fields, Object key){
    REQUIRE_TYPE(fields, t_table)
    Object field=table_get(E, fields.tp, key);
    if(field.type!=t_table){
        return null_const;// there is no proper field table at this key
    }

    int field_offset;
    GET_INT(field_offset, table_get(E, field.tp, to_string("offset")));

    char* field_position=(char*)position+field_offset;
    return field_get(E, field_position, field);
}


Object struct_descriptor_stringify(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];
    REQUIRE_TYPE(self, t_table)
    stream s;
    stream_init(&s, 256);

    int type;
    GET_INT(type, table_get(E, self.tp, to_string("type")));
    Object position=table_get(E, self.tp, to_string("position"));
    REQUIRE_TYPE(position, t_pointer)
    stream_printf(&s, "%s(%#lx)\n[\n\t", NATIVE_TYPES_NAMES[type], (long unsigned)position.p);
    if(type==n_struct){
        Object fields=table_get(E, self.tp, to_string("fields"));
        REQUIRE_TYPE(fields, t_table);
        TableIterator it=table_get_iterator(fields.tp);
        bool first=true;
        for(IterationResult i=table_iterator_next(&it); !i.finished; i=table_iterator_next(&it)) {
            if(first){
                first=false;
            } else {
                stream_push_const_string(&s, ", \n\t");
            }
            if(i.key.type==t_string){
                stream_push_string_indented(&s, i.key.text);
            } else {
                USING_STRING(stringify(E, i.key),
                    stream_push_string_indented(&s, str));
            }
            Object value=get(E, self, i.key);
            if(value.type==t_null){
                value=i.value;
            }
            char* stringified_value=stringify(E, value);
            stream_push_const_string(&s, "=");
            stream_push_string_indented(&s, stringified_value);
            free(stringified_value);
        }
    }

    stream_push_const_string(&s, "\n]\0");
    return to_string((char*)stream_get_data(&s));
}

Object struct_descriptor_get(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object key=arguments[1];

    REQUIRE_TYPE(self, t_table)

    // let symbol overrides through
    if(!(key.type==t_string || key.type==t_int)){
        Object result=table_get(E, self.tp, key);
        reference(&result);
        return result;
    }

    int type;
    GET_INT(type, table_get(E, self.tp, to_string("type")));
    Object position=table_get(E, self.tp, to_string("position"));
    REQUIRE_TYPE(position, t_pointer);

    // zero index refers to self
    if(key.type==t_int && key.int_value==0) {
        return field_get(E, position.p, self);
    } else if(type==n_struct){
        Object fields=table_get(E, self.tp, to_string("fields"));
        return struct_get_field(E, position.p, fields, key);
    } else {
        RETURN_ERROR("STRUCT_GET_ERROR", self, "Can't get field");
    }
}

static Object struct_set_field(Executor* E, void* position, Object fields, Object key, Object value);

Object field_set(Executor* E, void* position, Object field, Object value){
    int type;
    GET_INT(type, table_get(E, field.tp, to_string("type")));
    
    switch(type){
        // TODO: error if value isn't a number
        #define X(t) \
        case n_##t: \
            REQUIRE_TYPE(value, t_int) \
            *((t*)position)=(t)value.int_value; \
            break;
        INT_TYPES
        #undef X
        case n_float:
            REQUIRE_TYPE(value, t_float)
            *((float*)position)=value.float_value;
            break;
        case n_double:
            REQUIRE_TYPE(value, t_float)
            *((double*)position)=value.float_value;
            break;
        case n_long_double:
            REQUIRE_TYPE(value, t_float)
            *((long double*)position)=value.float_value;
            break;
        case n_string:
            REQUIRE_TYPE(value, t_string)
            if(*((char**)position)!=NULL){
                free(*((char**)position));
            }
            *((char**)position)=strdup(value.text);
            break;
        case n_struct:
        {
            Object it;
            Object fields=get(E, field, to_string("fields"));
            // you should iterate over fields instead to avoid setting excess values
            FOREACH(value, it, 
                struct_set_field(E, position, fields, get(E, it, to_string("key")),  get(E, it, to_string("value")));
            )
            break;
        }
        case n_pointer:
        {
            void** pointed=(void**)position;
            if(is_struct_descriptor(E, value)){
                table_set(E, field.tp, to_string("pointed"), value);
                *pointed=struct_descriptor_get_pointer(E, value.tp);
            } else {
                field_set(E, *pointed, table_get(E, field.tp, to_string("pointed")), value);
            }
            break;
        }
        default: RETURN_ERROR("STRUCTURE_SET_ERROR", field, "Field has incorrect type value %i.", type);
    }
    return value;
}

Object struct_set_field(Executor* E, void* position, Object fields, Object key, Object value) {
    Object field=table_get(E, fields.tp, key);

    int field_offset;
    GET_INT(field_offset, get(E, field, to_string("offset")));

    char* field_position=(char*)position+field_offset;
    return field_set(E, field_position, field, value);
}

Object struct_descriptor_set(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object key=arguments[1];
    Object value=arguments[2];

    REQUIRE_TYPE(self, t_table);

    int type;
    GET_INT(type, table_get(E, self.tp, to_string("type")));

    Object position=table_get(E, self.tp, to_string("position"));
    REQUIRE_TYPE(position, t_pointer);

    // 0 key in pointer refers to the pointed value itself
    if(key.type==t_int && key.int_value==0) {
        return field_set(E, position.p, self, value);
    }
    if(type==n_struct) {
        Object fields=table_get(E, self.tp, to_string("fields"));
        return struct_set_field(E, position.p, fields, key, value);
    } else {
         RETURN_ERROR("STRUCT_SET_ERROR", self, "Can't set field");
    }
}

void add_struct_descriptor_fields(Executor* E, Table* sd, void* position){
    table_set(E, sd, to_string("position"), to_pointer(position));
    table_set(E, sd, to_string("is_struct_descriptor"), to_int(1));
    
    table_set(E, sd, OVERRIDE(E, stringify), to_native_function(E, struct_descriptor_stringify, NULL, 1, false));
    table_set(E, sd, OVERRIDE(E, get), to_native_function(E, struct_descriptor_get, NULL, 2, false));
    table_set(E, sd, OVERRIDE(E, set), to_native_function(E, struct_descriptor_set, NULL, 3, false));
}

Object new_struct_descriptor(Executor* E, void* position, Object fields){
    Object sd;
    table_init(E, &sd);
    table_set(E, sd.tp, to_string("fields"), copy(E, fields));
    table_set(E, sd.tp, to_string("type"), to_int(n_struct));
    add_struct_descriptor_fields(E, sd.tp, position);
    return sd;
}

Object to_field(Executor* E, int offset, NativeType type){
    Object field;
    table_init(E, &field);
    set(E, field, to_string("offset"), to_int(offset));
    set(E, field, to_string("type"), to_int(type));
    return field;
}

Object to_struct_field(Executor* E, int offset, Object fields){
    Object struct_field=to_field(E, offset, n_struct);
    set(E, struct_field, to_string("fields"), fields);
    return struct_field;
}

Object to_struct_pointer_field(Executor* E, int offset, Object fields) {
    Object struct_pointer_field=to_field(E, offset, n_pointer);
    set(E, struct_pointer_field, to_string("pointed"), to_struct_field(E, 0, fields));
    return struct_pointer_field;
}

void* struct_descriptor_get_pointer(Executor* E, Table* sd){
    Object pointer=table_get(E, sd, to_string("position"));
    if(pointer.type!=t_pointer){
        USING_STRING(stringify(E, pointer),
            CRITICAL_ERROR(WRONG_ARGUMENT_TYPE, "Position field of struct descriptor should be of type pointer, it is %s", str))
    }
    return pointer.p;
}