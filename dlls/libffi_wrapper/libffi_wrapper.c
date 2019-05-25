#include "libffi_wrapper.h"

// ffi doesn't have definitions for int and char, so here they are defined:
#if int==int8_t
    #define ffi_type_int ffi_type_sint8
#elif int==int16_t
    #define ffi_type_int ffi_type_sint16
#elif int== int32_t
    #define ffi_type_int ffi_type_sint32
#elif int== int64_t
    #define ffi_type_int ffi_type_sint64
#else
    #define ffi_type_int NULL // int doesn't equal to any type from stdint, something is seriously wrong
#endif

#if char==int8_t
    #define ffi_type_char ffi_type_sint8
    #define FFI_TYPE_CHAR FFI_TYPE_SINT8
#elif char==int16_t
    #define ffi_type_char ffi_type_sint16
    #define FFI_TYPE_CHAR FFI_TYPE_SINT16
#elif char== int32_t
    #define ffi_type_char ffi_type_sint32
    #define FFI_TYPE_CHAR FFI_TYPE_SINT32
#elif char== int64_t
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

void* to_ffi_type(object o, ffi_type* t){
    switch(o.type){
        case t_string:
            return (void*)strdup(o.text);
        case t_number:
            switch(t->type){
                #define X(type, type_id, _) \
                case type_id: \
                { \
                    type* casted=malloc(sizeof(type)); \
                    *casted=o.value; \
                    return (void*)casted; \
                }
                
                NUMERIC_TYPES
                #undef X
            }
            // no struct support
    }
    return NULL;
}

object from_ffi_type(void* v, ffi_type* t){
    switch(t->type){
        #define X(type, type_id, _) \
        case type_id: \
            return to_number(*(type*)v);
        
        NUMERIC_TYPES
        #undef X
        // no struct support
    }
    return null_const;
}

object ffi_function_call(object* arguments, int arguments_count){
    object self=arguments[0];
    object cif_pointer=get(self, to_string("cif"));
    REQUIRE_TYPE(cif_pointer, t_pointer);

    object function_pointer=get(self, to_string("function"));
    REQUIRE_TYPE(function_pointer, t_pointer);

    ffi_cif* cif=(ffi_cif*)cif_pointer.p;

    void** values=malloc(sizeof(void*)*(arguments_count-1));
    for(int i=0; i<arguments_count-1; i++){
        values[i]=to_ffi_type(arguments[i+1], cif->arg_types[i]);
    }

    void* returned=malloc(cif->rtype->size);// hopefuly size works also on structs
    ffi_call(cif, FFI_FN(function_pointer.p), returned, values);

    object result=from_ffi_type(returned, cif->rtype);

    for(int i=0; i<arguments_count-1; i++){
        free(values[i]);
    }

    free(values);
    free(returned);
    
    return result;
}

object ffi_function_destroy(object* arguments, int arguments_count){
    object self=arguments[0];
    object cif_pointer=get(self, to_string("cif"));
    REQUIRE_TYPE(cif_pointer, t_pointer);

    ffi_cif* cif=(ffi_cif*)cif_pointer.p;
    free(cif->arg_types);
    free(cif);
}

void* ffi_find_function(object self, char* function_name){
    object lib_pointer=get(self, to_number(0));
    if(lib_pointer.type!=t_pointer){
        return NULL;
    }
    return find_symbol(lib_pointer.p, function_name);
}

object ffi_function(object* arguments, int arguments_count){
    object self=arguments[0];
    object function_name=arguments[1];
    REQUIRE_TYPE(function_name, t_string);
    object types=get(self, to_string("types"));

    int argument_types_count=arguments_count-3;
    ffi_type** argument_types=malloc(sizeof(ffi_type)*(argument_types_count));
    for(int i=0; i<argument_types_count; i++){
        object argument_type=get(types, arguments[2+i]);
        REQUIRE_TYPE(argument_type, t_pointer);
        argument_types[i]=(ffi_type*)argument_type.p;
    }
    object return_argument_type=get(types, arguments[arguments_count-1]);
    REQUIRE_TYPE(return_argument_type, t_pointer);
    ffi_type* return_type=(ffi_type*)return_argument_type.p;

    ffi_cif* cif=malloc(sizeof(ffi_cif));
    ffi_status status = ffi_prep_cif(cif, FFI_DEFAULT_ABI, argument_types_count, return_type,
                                     argument_types);

    if (status != FFI_OK) {
        RETURN_ERROR("FFI_ERROR", arguments[0], "ffi_prep_cif failed: %d\n", status);
    }

    object result;
    table_init(&result);
    set(result, to_string("cif"), to_pointer(cif));
    set(result, to_string("function"), to_pointer(ffi_find_function(self, function_name.text)));
    set_function(result, "call", 1, true, ffi_function_call);
    set_function(result, "destroy", 1, false, ffi_function_destroy);
    printf("5");
    return result;
}

object ffi_destroy(object* arguments, int arguments_count){
    object self=arguments[0];
    object lib_pointer=get(self, to_number(0));
    REQUIRE_TYPE(lib_pointer, t_pointer);
    close_dll(lib_pointer.p);
}

object ffi_open(object* arguments, int arguments_count){
    object library_name=arguments[0];
    object result;
    table_init(&result);

    void* dll_handle;
    if(library_name.type==t_null){
        dll_handle=get_dll_handle(NULL);
    } else {
        REQUIRE_TYPE(library_name, t_string);
        dll_handle=get_dll_handle(library_name.text);
    }
    set(result, to_number(0), to_pointer(dll_handle));
    
    set_function(result, "function", 3, true, ffi_function);
    set_function(result, "destroy", 1, true, ffi_destroy);
    
    object types;
    table_init(&types);
    #define X(type, type_id, type_variable) \
        set(types, to_string(#type), to_pointer(&type_variable));

    NUMERIC_TYPES
    #undef X
    set(result, to_string("types"), types);
    
    return result;
}

object duck_module_init(){
    object module;
    table_init(&module);
    set_function(module, "open", 1, false, ffi_open);
    return module;
}

#undef NUMERIC_TYPES