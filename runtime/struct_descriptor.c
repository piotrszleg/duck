#include "struct_descriptor.h"

#define GET_INT(result_name, exp) \
    { \
        Object o=exp;\
        if(o.type!=t_number) { \
            RETURN_ERROR("INCORRECT_ARGUMENT_TYPE", o, "Field type should be number"); \
        } else { \
            result_name=(int)o.value; \
        } \
    }

// TODO: substructures, tests

static Object struct_set(Executor* E, Object* arguments, int arguments_count);
static Object struct_get(Executor* E, Object* arguments, int arguments_count);

Object pointer_get(Executor* E, void* position, Object field){
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
            set_table(E, field.tp, to_string("position"), to_pointer(position));
            set_table(E, field.tp, to_string("get"), to_function(E, struct_get, NULL, 2));
            set_table(E, field.tp, to_string("set"), to_function(E, struct_set, NULL, 3));
            return field;
        default: RETURN_ERROR("STRUCTURE_GET_ERROR", field, "No type was matched.");
    }
}

Object struct_get_(Executor* E, void* position, Object class, Object key){
    REQUIRE_TYPE(class, t_table)
    Object field=get_table(class.tp, key);
    REQUIRE_TYPE(field, t_table)

    int field_offset;
    GET_INT(field_offset, get_table(field.tp, to_string("offset")));

    char* field_position=(char*)position+field_offset;
    return pointer_get(E, field_position, field);
}

Object struct_get(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object key=arguments[1];

    REQUIRE_TYPE(self, t_table)

    int type;
    GET_INT(type, get_table(self.tp, to_string("type")));
    Object position=get_table(self.tp, to_string("position"));
    REQUIRE_TYPE(position, t_pointer);

    if(type==n_struct) {
        // get field in pointed value
        Object class=get_table(self.tp, to_string("class"));
        return struct_get_(E, position.p, class, key);
    } else if(type==n_pointer) {
        void* pointed=*(void**)position.p;
        Object pointed_field=get_table(self.tp, to_string("pointed"));
        REQUIRE_TYPE(pointed_field, t_table)
        if(key.type==t_number && key.value==0) {
            return pointer_get(E, pointed, pointed_field);
        } else {
            Object class=get_table(pointed_field.tp, to_string("class"));
            return struct_get_(E, pointed, class, key);
        }
    } else {
        RETURN_ERROR("STRUCT_GET_ERROR", self, "Can't get field");
    }
}

static Object struct_set_(Executor* E, void* position, Object class, Object key, Object value);

Object pointer_set(Executor* E, void* position, Object field, Object value){
    int type;
    GET_INT(type, get_table(field.tp, to_string("type")));
    
    switch(type){
        // TODO: error if value isn't a number
        case n_int:
            REQUIRE_TYPE(value, t_number)
            *((int*)position)=(int)value.value;
            break;
        case n_float:
            REQUIRE_TYPE(value, t_number)
            *((float*)position)=value.value;
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
            Object class=get(E, field, to_string("class"));
            // you should iterate over class instead to avoid setting eccess values
            FOREACH(value, it, 
                struct_set_(E, position, class, get(E, it, to_string("key")),  get(E, it, to_string("value")));
            )
            break;
        }
        case n_pointer:
        {
            void* pointed=*(void**)position;
            pointer_set(E, pointed, get(E, field, to_string("pointed")), value);
            break;
        }
        default: RETURN_ERROR("STRUCTURE_SET_ERROR", field, "No type was matched.");
    }
    return value;
}

Object struct_set_(Executor* E, void* position, Object class, Object key, Object value) {
    Object field=get_table(class.tp, key);

    int field_offset;
    GET_INT(field_offset, get(E, field, to_string("offset")));

    char* field_position=(char*)position+field_offset;
    return pointer_set(E, field_position, field, value);
}

Object struct_set(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object key=arguments[1];
    Object value=arguments[2];

    REQUIRE_TYPE(self, t_table);

    int type;
    GET_INT(type, get_table(self.tp, to_string("type")));

    Object position=get_table(self.tp, to_string("position"));
    REQUIRE_TYPE(position, t_pointer);

    // 0 key in pointer refers to the pointed value itself
    if(type==n_struct) {
        Object class=get_table(self.tp, to_string("class"));
        return struct_set_(E, position.p, class, key, value);
    } else if(type==n_pointer){
        void* pointed=*(void**)position.p;
        Object field=get_table(self.tp, to_string("pointed"));
        REQUIRE_TYPE(field, t_table)
        if(key.type==t_number && key.value==0) {
            return pointer_set(E, pointed, field, value);
        } else {
            Object class=get_table(field.tp, to_string("class"));
            return struct_set_(E, pointed, class, key, value);
        }
    } else {
         RETURN_ERROR("STRUCT_SET_ERROR", self, "Can't set field");
    }
}

Object new_struct_descriptor(Executor* E, void* position, Object sclass){
    Object sd;
    table_init(E, &sd);
    set(E, sd, to_string("type"), to_number(n_struct));
    set(E, sd, to_string("position"), to_pointer(position));
    set(E, sd, to_string("class"), sclass);
    set(E, sd, to_string("get"), to_function(E, struct_get, NULL, 2));
    set(E, sd, to_string("set"), to_function(E, struct_set, NULL, 3));
    return sd;
}

Object to_field(Executor* E, int offset, NativeType type){
    Object field;
    table_init(E, &field);
    set(E, field, to_string("offset"), to_number(offset));
    set(E, field, to_string("type"), to_number(type));
    return field;
}

/*
// this needs to go into libffi module to handle allignment on different architectures
Object new_struct_descriptor_class(Object* arguments, int arguments_count){
    Object class;

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

int struct_size(Object st){

}

Object struct_descriptor_allocate(Object* arguments, int arguments_count){
    Object class=arguments[0];
    Object sd;
    void* allocated=malloc(struct_size(class));

    return new_struct_descriptor(allocated, class);
}*/