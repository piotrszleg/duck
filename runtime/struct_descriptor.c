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

static object struct_set(object* arguments, int arguments_count);
static object struct_get(object* arguments, int arguments_count);

object pointer_get(void* position, object field){
    int type;
    REQUIRE_TYPE(field, t_table)
    GET_INT(type, get_table(field.tp, to_string("type")));

    switch(type){
        // TODO: error if value isn't a number
        case n_int:
            return to_number(*(int*)position);
        case n_float:
            return to_number(*(float*)position);
        case n_string:
            return to_string(*(char**)position);
        case n_struct:
        case n_pointer:
            set_table(field.tp, to_string("position"), to_pointer(position));
            set_table(field.tp, to_string("get"), to_function(struct_get, NULL, 2));
            set_table(field.tp, to_string("set"), to_function(struct_set, NULL, 3));
            return field;
        default: RETURN_ERROR("STRUCTURE_GET_ERROR", field, "No type was matched.");
    }
}

object struct_get_(void* position, object class, object key){
    REQUIRE_TYPE(class, t_table)
    object field=get_table(class.tp, key);
    REQUIRE_TYPE(field, t_table)

    int field_offset;
    GET_INT(field_offset, get_table(field.tp, to_string("offset")));

    char* field_position=(char*)position+field_offset;
    return pointer_get(field_position, field);
}

object struct_get(object* arguments, int arguments_count){
    object self=arguments[0];
    object key=arguments[1];

    REQUIRE_TYPE(self, t_table)

    int type;
    GET_INT(type, get_table(self.tp, to_string("type")));
    object position=get_table(self.tp, to_string("position"));
    REQUIRE_TYPE(position, t_pointer);

    if(type==n_struct) {
        // get field in pointed value
        object class=get_table(self.tp, to_string("class"));
        return struct_get_(position.p, class, key);
    } else if(type==n_pointer) {
        void* pointed=*(void**)position.p;
        object pointed_field=get_table(self.tp, to_string("pointed"));
        REQUIRE_TYPE(pointed_field, t_table)
        if(key.type==t_number && key.value==0) {
            return pointer_get(pointed, pointed_field);
        } else {
            object class=get_table(pointed_field.tp, to_string("class"));
            return struct_get_(pointed, class, key);
        }
    } else {
        RETURN_ERROR("STRUCT_GET_ERROR", self, "Can't get field");
    }
}

static object struct_set_(void* position, object class, object key, object value);

object pointer_set(void* position, object field, object value){
    int type;
    GET_INT(type, get_table(field.tp, to_string("type")));
    
    switch(type){
        // TODO: error if value isn't a number
        case n_int:
            *((int*)position)=(int)value.value;
            break;
        case n_float:
            *((float*)position)=value.value;
            break;
        case n_string:
            *((char**)position)=stringify(value);
            break;
        case n_struct:
        {
            object it;
            object class=get(field, to_string("class"));
            // you should iterate over class instead to avoid setting eccess values
            FOREACH(value, it, 
                struct_set_(position, class, get(it, to_string("key")),  get(it, to_string("value")));
            )
            break;
        }
        case n_pointer:
        {
            void* pointed=*(void**)position;
            pointer_set(pointed, get(field, to_string("pointed")), value);
            break;
        }
        default: RETURN_ERROR("STRUCTURE_SET_ERROR", field, "No type was matched.");
    }
    return value;
}

object struct_set_(void* position, object class, object key, object value) {
    object field=get_table(class.tp, key);

    int field_offset;
    GET_INT(field_offset, get(field, to_string("offset")));

    char* field_position=(char*)position+field_offset;
    return pointer_set(field_position, field, value);
}

object struct_set(object* arguments, int arguments_count){
    object self=arguments[0];
    object key=arguments[1];
    object value=arguments[2];

    REQUIRE_TYPE(self, t_table);

    int type;
    GET_INT(type, get_table(self.tp, to_string("type")));

    object position=get_table(self.tp, to_string("position"));
    REQUIRE_TYPE(position, t_pointer);

    // 0 key in pointer refers to the pointed value itself
    if(type==n_struct) {
        object class=get_table(self.tp, to_string("class"));
        return struct_set_(position.p, class, key, value);
    } else if(type==n_pointer){
        void* pointed=*(void**)position.p;
        object field=get_table(self.tp, to_string("pointed"));
        REQUIRE_TYPE(field, t_table)
        if(key.type==t_number && key.value==0) {
            return pointer_set(pointed, field, value);
        } else {
            object class=get_table(field.tp, to_string("class"));
            return struct_set_(pointed, class, key, value);
        }
    } else {
         RETURN_ERROR("STRUCT_SET_ERROR", self, "Can't set field");
    }
}

object new_struct_descriptor(void* position, object sclass){
    object sd;
    table_init(&sd);
    set(sd, to_string("type"), to_number(n_struct));
    set(sd, to_string("position"), to_pointer(position));
    set(sd, to_string("class"), sclass);
    set(sd, to_string("get"), to_function(struct_get, NULL, 2));
    set(sd, to_string("set"), to_function(struct_set, NULL, 3));
    return sd;
}

object to_field(int offset, native_type type){
    object field;
    table_init(&field);
    set(field, to_string("offset"), to_number(offset));
    set(field, to_string("type"), to_number(type));
    return field;
}

/*
// this needs to go into libffi module to handle allignment on different architectures
object new_struct_descriptor_class(object* arguments, int arguments_count){
    object class;

    for(int i=0; i<arguments_count; i++){
        REQUIRE_TYPE(arguments[i], t_string);
        char* type_name=arguments[i].text;

        char** NATIVE_TYPE_NAMES;
        int NATIVE_TYPE_NAMES_COUNT=10;
        for(int i=0; i<NATIVE_TYPE_NAMES_COUNT; i++){
            if(strcmp(NATIVE_TYPE_NAMES[i], type_name)==0){
                
            }
        }
    }

    return class;
}

int struct_size(object st){

}

object struct_descriptor_allocate(object* arguments, int arguments_count){
    object class=arguments[0];
    object sd;
    void* allocated=malloc(struct_size(class));

    return new_struct_descriptor(allocated, class);
}*/