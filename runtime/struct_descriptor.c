#include "struct_descriptor.h"

#define GET_INT(result_name, exp) \
    { \
        object o=exp;\
        if(o.type!=t_number) { \
            RETURN_ERROR("INCORRECT_ARGUMENT_TYPE", o, "Field type should be number"); \
        } else { \
            result_name=(int)o.value; \
        } \
    }

// TODO: substructures, tests

object struct_get(object* arguments, int arguments_count){
    object self=arguments[0];
    object key=arguments[1];

    object class=get_table(self.tp, to_string("class"));
    object field=get_table(class.tp, key);
    if(field.type==t_null){
        return field;
    }

    int position, field_offset, field_type;
    GET_INT(position, get_table(self.tp, to_string("position")));
    GET_INT(field_offset, get_table(field.tp, to_string("offset")));
    GET_INT(field_type, get_table(field.tp, to_string("type")));
    char* target_struct=(char*)position;
    char* field_position=target_struct+field_offset/sizeof(char);
    switch(field_type){
        // TODO: error if value isn't a number
        case n_int:
            return to_number(*(int*)field_position);
        case n_float:
            return to_number(*(float*)field_position);
        case n_string:
            return to_string(*(char**)field_position);
        default: return null_const;
    }
}

object struct_set(object* arguments, int arguments_count){
    object self=arguments[0];
    object key=arguments[1];
    object value=arguments[2];

    object class=get_table(self.tp, to_string("class"));
    object field=get_table(class.tp, key);

    int position, field_offset, field_type;
    GET_INT(position, get_table(self.tp, to_string("position")));
    GET_INT(field_offset, get(field, to_string("offset")));
    GET_INT(field_type, get(field, to_string("type")));
    char* target_struct=(char*)position;
    char* field_position=target_struct+field_offset/sizeof(char);
    switch(field_type){
        // TODO: error if value isn't a number
        case n_int:
            *((int*)field_position)=(int)value.value;
            break;
        case n_float:
            *((float*)field_position)=value.value;
            break;
        case n_string:
            *((char**)field_position)=stringify(value);
            break;
        case n_struct:
        {
            //struct_set_(field_position, get(field, to_string("class")), key, value);
            // for each key in value call set with class of the struct as an argument
            break;
        }
    }
    return value;
}

object new_struct_descriptor(void* position, object sclass){
    object sd;
    table_init(&sd);
    set(sd, to_string("position"), to_number((int)position));
    set(sd, to_string("class"), sclass);
    set(sd, to_string("get"), to_function(struct_get, NULL, 2));
    set(sd, to_string("set"), to_function(struct_set, NULL, 3));
    return sd;
}

object to_field(int offset, native_type type){
    object field;
    table_init(&field);
    set(field, to_string("offset"), to_number((int)offset));
    set(field, to_string("type"), to_number(type));
    return field;
}