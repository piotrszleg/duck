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

static Object struct_descriptor_set(Executor* E, Object* arguments, int arguments_count);
static Object struct_descriptor_get(Executor* E, Object* arguments, int arguments_count);

bool is_struct_descriptor(Executor* E, Object o){
    if(o.type!=t_table){
        return false;
    }
    Object is_struct_descriptor=get_table(o.tp, to_string("is_struct_descriptor"));
    bool result=!is_falsy(is_struct_descriptor);
    destroy_unreferenced(E, &is_struct_descriptor);
    return result;
}

Object field_get(Executor* E, void* position, Object field){
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
            return new_struct_descriptor(E, position, get_table(field.tp, to_string("class")));
        case n_pointer:
        {
            void* pointed_position=*(void**)position;
            Object pointed=get_table(field.tp, to_string("pointed"));
            REQUIRE_TYPE(pointed, t_table)
            return new_struct_descriptor(E, pointed_position, get_table(pointed.tp, to_string("class")));
        }
        default: RETURN_ERROR("STRUCTURE_GET_ERROR", field, "Field has incorrect type value %i.", type);
    }
}

Object struct_get_field(Executor* E, void* position, Object class, Object key){
    REQUIRE_TYPE(class, t_table)
    Object field=get_table(class.tp, key);
    REQUIRE_TYPE(field, t_table)

    int field_offset;
    GET_INT(field_offset, get_table(field.tp, to_string("offset")));

    char* field_position=(char*)position+field_offset;
    return field_get(E, field_position, field);
}

Object struct_descriptor_get(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object key=arguments[1];

    REQUIRE_TYPE(self, t_table)

    int type;
    GET_INT(type, get_table(self.tp, to_string("type")));
    Object position=get_table(self.tp, to_string("position"));
    REQUIRE_TYPE(position, t_pointer);

    // zero index refers to self
    if(key.type==t_number && key.value==0) {
        return field_get(E, position.p, self);
    } else if(type==n_struct){
        Object class=get_table(self.tp, to_string("class"));
        return struct_get_field(E, position.p, class, key);
    } else {
        RETURN_ERROR("STRUCT_GET_ERROR", self, "Can't get field");
    }
}

static Object struct_set_field(Executor* E, void* position, Object class, Object key, Object value);

Object field_set(Executor* E, void* position, Object field, Object value){
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
                struct_set_field(E, position, class, get(E, it, to_string("key")),  get(E, it, to_string("value")));
            )
            break;
        }
        case n_pointer:
        {
            void** pointed=(void**)position;
            if(is_struct_descriptor(E, value)){
                *pointed=struct_descriptor_get_pointer(E, value.tp);
            } else {
                field_set(E, *pointed, get(E, field, to_string("pointed")), value);
            }
            break;
        }
        default: RETURN_ERROR("STRUCTURE_SET_ERROR", field, "Field has incorrect type value %i.", type);
    }
    return value;
}

Object struct_set_field(Executor* E, void* position, Object class, Object key, Object value) {
    Object field=get_table(class.tp, key);

    int field_offset;
    GET_INT(field_offset, get(E, field, to_string("offset")));

    char* field_position=(char*)position+field_offset;
    return field_set(E, field_position, field, value);
}

Object struct_descriptor_set(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object key=arguments[1];
    Object value=arguments[2];

    REQUIRE_TYPE(self, t_table);

    int type;
    GET_INT(type, get_table(self.tp, to_string("type")));

    Object position=get_table(self.tp, to_string("position"));
    REQUIRE_TYPE(position, t_pointer);

    // 0 key in pointer refers to the pointed value itself
    if(key.type==t_number && key.value==0) {
        field_set(E, position.p, self, value);
    }
    if(type==n_struct) {
        Object class=get_table(self.tp, to_string("class"));
        return struct_set_field(E, position.p, class, key, value);
    } else {
         RETURN_ERROR("STRUCT_SET_ERROR", self, "Can't set field");
    }
}

Object new_struct_descriptor(Executor* E, void* position, Object sclass){
    Object sd;
    table_init(E, &sd);
    set(E, sd, to_string("type"), to_number(n_struct));
    set(E, sd, to_string("position"), to_pointer(position));
    set(E, sd, to_string("is_struct_descriptor"), to_number(1));
    set(E, sd, to_string("class"), sclass);
    set(E, sd, to_string("get"), to_function(E, struct_descriptor_get, NULL, 2));
    set(E, sd, to_string("set"), to_function(E, struct_descriptor_set, NULL, 3));
    return sd;
}

Object to_field(Executor* E, int offset, NativeType type){
    Object field;
    table_init(E, &field);
    set(E, field, to_string("offset"), to_number(offset));
    set(E, field, to_string("type"), to_number(type));
    return field;
}

Object to_struct_field(Executor* E, int offset, Object class){
    Object struct_field=to_field(E, offset, n_struct);
    set(E, struct_field, to_string("class"), class);
    return struct_field;
}

Object to_struct_pointer_field(Executor* E, int offset, Object class) {
    Object struct_pointer_field=to_field(E, offset, n_pointer);
    set(E, struct_pointer_field, to_string("pointed"), to_struct_field(E, 0, class));
    return struct_pointer_field;
}

void* struct_descriptor_get_pointer(Executor* E, Table* sd){
    Object pointer=get_table(sd, to_string("position"));
    if(pointer.type!=t_pointer){
        USING_STRING(stringify(E, wrap_gc_object((gc_Object*)sd)),
            THROW_ERROR(WRONG_ARGUMENT_TYPE, "Position field of struct descriptor should be of type pointer, it is %s", str))
    }
    return pointer.p;
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
