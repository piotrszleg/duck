#include "libffi_wrapper.h"

// ffi doesn't have definitions for int and char, so here they are defined:
#if INT_MAX==INT8_MAX
    #define ffi_type_int ffi_type_sint8
#elif INT_MAX==INT16_MAX
    #define ffi_type_int ffi_type_sint16
#elif INT_MAX==INT32_MAX
    #define ffi_type_int ffi_type_sint32
#elif INT_MAX== INT64_MAX
    #define ffi_type_int ffi_type_sint64
#else
    #define ffi_type_int NULL // int doesn't equal to any type from stdint, something is seriously wrong
#endif

#if CHAR_MAX==INT8_MAX
    #define ffi_type_char ffi_type_sint8
    #define FFI_TYPE_CHAR FFI_TYPE_SINT8
#elif CHAR_MAX==INT16_MAX
    #define ffi_type_char ffi_type_sint16
    #define FFI_TYPE_CHAR FFI_TYPE_SINT16
#elif CHAR_MAX==INT32_MAX
    #define ffi_type_char ffi_type_sint32
    #define FFI_TYPE_CHAR FFI_TYPE_SINT32
#elif CHAR_MAX==INT64_MAX
    #define ffi_type_char ffi_type_sint64
    #define FFI_TYPE_CHAR FFI_TYPE_SINT64
#else
    #define ffi_type_char NULL // char doesn't equal to any type from stdint, something is seriously wrong
    #define FFI_TYPE_CHAR NULL
#endif

#define NUMERIC_TYPES \
    X(float,       FFI_TYPE_FLOAT,      ffi_type_float       ) \
    X(double,      FFI_TYPE_DOUBLE,     ffi_type_double      ) \
    X(long double, FFI_TYPE_LONGDOUBLE, ffi_type_longdouble  ) \
    X(int,         FFI_TYPE_INT,        ffi_type_int         ) \
    /*X(char,        FFI_TYPE_CHAR,       ffi_type_char        )*/ \
    X(uint8_t,     FFI_TYPE_UINT8,      ffi_type_uint8       ) \
    X(int8_t,      FFI_TYPE_SINT8,      ffi_type_sint8       ) \
    X(uint16_t,    FFI_TYPE_UINT16,     ffi_type_uint16      ) \
    X(int16_t,     FFI_TYPE_SINT16,     ffi_type_sint16      ) \
    X(uint32_t,    FFI_TYPE_UINT32,     ffi_type_uint32      ) \
    X(int32_t,     FFI_TYPE_SINT32,     ffi_type_sint32      ) \
    X(uint64_t,    FFI_TYPE_UINT64,     ffi_type_uint64      ) \
    X(int64_t,     FFI_TYPE_SINT64,     ffi_type_sint64      ) \
    X(long long,   FFI_TYPE_POINTER,    ffi_type_pointer     )
    // leaving out complex numbers

void* to_ffi_type(Executor* E, Object o, ffi_type* t, Object type_object){
    switch(o.type){
        case t_string:
            return (void*)strdup(o.text);
        case t_number:
            switch(t->type){
                #define X(type, type_id, _) \
                case type_id: \
                { \
                    type* casted=malloc(sizeof(type)); \
                    CHECK_ALLOCATION(casted) \
                    *casted=o.value; \
                    return (void*)casted; \
                }
                
                NUMERIC_TYPES
                #undef X
            }
            break;
        case t_table:
        {
            char* result=malloc(sizeof(t->size));
            CHECK_ALLOCATION(result)
            int position=0;
            for(int i=0; t->elements[i]!=NULL; i++){
                Object field_object=get(E, type_object, to_number(i));
                Object field_name=get(E, field_object, to_number(0));
                Object field_type=get(E, field_object, to_number(1));
                Object field_value=get(E, o, field_name);
                int size=t->elements[i]->size;
                void* converted_field=to_ffi_type(E, field_value, t->elements[i], field_type);
                memcpy(result+position, converted_field, size);
                free(converted_field);
                position+=size;
            }
            return result;
        }
    }
    USING_STRING(stringify(E, o),
        THROW_ERROR(WRONG_ARGUMENT_TYPE, "Couldn't convert %s to ffi type.", str));
    return NULL;
}

Object from_ffi_type(Executor* E, void* v, ffi_type* t, Object type_object){
    switch(t->type){
        #define X(type, type_id, _) \
        case type_id: \
            return to_number(*(type*)v);
        
        NUMERIC_TYPES

        #undef X
        case FFI_TYPE_STRUCT:
        {
            Object o;
            table_init(&o);
            int position=0;
            for(int i=0; t->elements[i]!=NULL; i++){
                void* field=(char*)v+position;
                Object field_object=get(E, type_object, to_number(i));
                Object field_name=get(E, field_object, to_number(0));
                Object field_type=get(E, field_object, to_number(1));
                set(E, o, field_name, from_ffi_type(E, field, t->elements[i], field_type));
                int size=t->elements[i]->size;
                position+=size;
            }
            return o;
        }
    }
    return null_const;
}

Object ffi_function_call(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object cif_pointer=get(E, self, to_string("cif"));
    REQUIRE_TYPE(cif_pointer, t_pointer);

    Object function_pointer=get(E, self, to_string("function"));
    REQUIRE_TYPE(function_pointer, t_pointer);

    ffi_cif* cif=(ffi_cif*)cif_pointer.p;

    void** values=malloc(sizeof(void*)*(arguments_count-1));
    CHECK_ALLOCATION(values);
    for(int i=0; i<arguments_count-1; i++){
        Object type_object=get(E, self, to_number(i));
        values[i]=to_ffi_type(E, arguments[i+1], cif->arg_types[i], type_object);
    }
    void* returned=malloc(cif->rtype->size);// hopefuly size works also on structs
    ffi_call(cif, FFI_FN(function_pointer.p), returned, values);

    Object result=from_ffi_type(E, returned, cif->rtype, get(E, self, to_number(arguments_count-1)));

    for(int i=0; i<arguments_count-1; i++){
        free(values[i]);
    }

    free(values);
    free(returned);
    
    return result;
}

Object ffi_function_destroy(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object cif_pointer=get(E, self, to_string("cif"));
    REQUIRE_TYPE(cif_pointer, t_pointer);

    ffi_cif* cif=(ffi_cif*)cif_pointer.p;
    for(int i=0; i<cif->nargs; i++){
        free(cif->arg_types[i]);
    }
    free(cif->arg_types);
    free(cif);
    return null_const;
}

void* ffi_find_function(Executor* E, Object self, char* function_name){
    Object lib_pointer=get(E, self, to_number(0));
    if(lib_pointer.type!=t_pointer){
        return NULL;
    }
    return find_symbol(lib_pointer.p, function_name);
}

#define GET_FFI_TYPE(type_object, basic_types_table, result) \
    { \
        Object ffi_type_pointer; \
        if((type_object).type==t_string){ \
            /*if provided argument type is a string use it as a key
            to get one of the basic types in types dictionary*/ \
            ffi_type_pointer=get(E, (basic_types_table), (type_object)); \
        } else { \
            /* treat argument as a Table returned by ffi_struct function */ \
            ffi_type_pointer=get(E, (type_object), to_string("type")); \
        } \
        REQUIRE_TYPE(ffi_type_pointer, t_pointer); \
        (result)=(ffi_type*)ffi_type_pointer.p; \
    }

#define DEBUG_PRINT printf("<%i>\n", __COUNTER__);

Object ffi_function(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object function_name=arguments[1];
    REQUIRE_TYPE(function_name, t_string);
    Object types=get(E, self, to_string("types"));
 
    Object result;
    table_init(&result);

    int argument_types_count=arguments_count-3;
    ffi_type** argument_types=malloc(sizeof(ffi_type*)*(argument_types_count));
    CHECK_ALLOCATION(argument_types)
    for(int i=0; i<argument_types_count; i++){
        GET_FFI_TYPE(arguments[2+i], types, argument_types[i]);
        set(E, result, to_number(i), arguments[2+i]);
    }
    ffi_type* return_type;
    GET_FFI_TYPE(arguments[arguments_count-1], types, return_type);
    set(E, result, to_number(argument_types_count), arguments[arguments_count-1]);

    ffi_cif* cif=malloc(sizeof(ffi_cif));
    CHECK_ALLOCATION(cif)
    
    ffi_status status = ffi_prep_cif(cif, FFI_DEFAULT_ABI, argument_types_count, return_type,
                                     argument_types);

    if (status != FFI_OK) {
        RETURN_ERROR("FFI_ERROR", arguments[0], "ffi_prep_cif failed: %d\n", status);
    }

    set(E, result, to_string("cif"), to_pointer(cif));
    set(E, result, to_string("function"), to_pointer(ffi_find_function(E, self, function_name.text)));
    set_function(E, result, "call", 1, true, ffi_function_call);
    set_function(E, result, "destroy", 1, false, ffi_function_destroy);

    return result;
}

Object ffi_struct_destroy(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object type_pointer=get(E, self, to_string("type"));
    REQUIRE_TYPE(type_pointer, t_pointer);

    ffi_type* struct_type=(ffi_type*)type_pointer.p;
    for(int i=0; struct_type->elements[i]!=NULL; i++){
        free(struct_type->elements[i]);
    }
    free(struct_type->elements);
    free(struct_type);
    return null_const;
}

Object ffi_struct(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object types=get(E, self, to_string("types"));

    ffi_type* struct_type=malloc(sizeof(ffi_type));
    CHECK_ALLOCATION(struct_type)
    struct_type->size = struct_type->alignment = 0;
    struct_type->type = FFI_TYPE_STRUCT;
    struct_type->elements = malloc(sizeof(ffi_type*)*arguments_count);
    CHECK_ALLOCATION(struct_type->elements)

    Object result;
    table_init(&result);
    set(E, result, to_string("type"), to_pointer(struct_type));

    for(int i=1; i<arguments_count; i++){
        // fields passed as arguments to this function are in format:
        // [field_name, field_type] 
        Object field_type=get(E, arguments[i], to_number(1));
        GET_FFI_TYPE(field_type, types, struct_type->elements[i-1]);

        set(E, result, to_number(i-1), arguments[i]);
    }
    struct_type->elements[arguments_count]=NULL;

    set_function(E, result, "destroy", 1, false, ffi_struct_destroy);
    return result;
}

Object ffi_destroy(Executor* E, Object* arguments, int arguments_count){
    Object self=arguments[0];
    Object lib_pointer=get(E, self, to_number(0));
    REQUIRE_TYPE(lib_pointer, t_pointer);
    close_dll(lib_pointer.p);
    return null_const;
}

Object ffi_open(Executor* E, Object* arguments, int arguments_count){
    Object library_name=arguments[0];
    Object result;
    table_init(&result);

    void* dll_handle;
    if(library_name.type==t_null){
        dll_handle=get_dll_handle(NULL);
    } else {
        REQUIRE_TYPE(library_name, t_string);
        dll_handle=get_dll_handle(library_name.text);
    }
    set(E, result, to_number(0), to_pointer(dll_handle));
    
    set_function(E, result, "struct", 1, true, ffi_struct);
    set_function(E, result, "function", 2, true, ffi_function);
    set_function(E, result, "destroy", 1, false, ffi_destroy);
    
    Object types;
    table_init(&types);
    #define X(type, type_id, type_variable) \
        set(E, types, to_string(#type), to_pointer(&type_variable));

    NUMERIC_TYPES
    #undef X
    set(E, result, to_string("types"), types);
    
    return result;
}

Object duck_module_init(Executor* E){
    Object module;
    table_init(&module);
    set_function(E, module, "open", 1, false, ffi_open);
    return module;
}

#undef NUMERIC_TYPES