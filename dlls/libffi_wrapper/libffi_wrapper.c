#include "ffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "../../object_system/object.h"
#include "../../datatypes/map.h"
#include "../../runtime/import_dll.h"
#include "../../execution.h"

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
    X(int64_t,     FFI_TYPE_SINT64,     ffi_type_sint64      )
    // leaving out complex numbers

ffi_type* get_ffi_type_identifier(Executor* E, Object type_object, Object basic_types) {
    Object result;
    if(type_object.type==t_string){
        // if provided argument type is a string use it as a key
        // to get one of the basic types in basic_types table
        result=get(E, basic_types, type_object);
    } else {
        Object first_field=get(E, type_object, to_int(0));
        if(first_field.type==t_string && (strcmp(first_field.text, "pointer")==0 ||  strcmp(first_field.text, "array")==0)) {
            result=to_pointer(&ffi_type_pointer);
        } else {
            // treat argument as a Table returned by ffi_struct function
            result=get(E, type_object, to_string("type"));
        }
        destroy_unreferenced(E, &first_field);
    }
    if(result.type!=t_pointer){
        return NULL;
    } else {
        return result.p;
    }
}

int count_array_part(Executor* E, Object o){
    int i=-1;
    Object get_result=null_const;
    do{
        i++;
        destroy_unreferenced(E, &get_result);
        get_result=get(E, o, to_int(i));
    } while(get_result.type!=t_null);
    destroy_unreferenced(E, &get_result);
    return i;
}

Object to_struct_descriptor(Executor* E, void* to_convert, ffi_type* ffi_tp, Object type_object){
    Object fields;
    table_init(E, &fields);
    int position=0;
    for(int i=0; ffi_tp->elements[i]!=NULL; i++){
        Object field_object=get(E, type_object, to_int(i));
        Object field_name=get(E, field_object, to_int(0));
        Object field_type=get(E, field_object, to_int(1));

        if(EQUALS_STRING(field_type, "string")){
            set(E, fields, field_name, to_field(E, position, n_string));
        }
        else if(EQUALS_STRING(field_type, "int")){
            set(E, fields, field_name, to_field(E, position, n_int));
        }
        else if(EQUALS_STRING(field_type, "double")){
            set(E, fields, field_name, to_field(E, position, n_double));
        }
        else if(EQUALS_STRING(field_type, "long double")){
            set(E, fields, field_name, to_field(E, position, n_long_double));
        }
        #define X(t) \
        else if(EQUALS_STRING(field_type, #t)){ \
            set(E, fields, field_name, to_field(E, position, n_##t)); \
        }
        INT_TYPES
        #undef X
        else {
            RETURN_ERROR("FFI_ERROR", field_object, "Struct descriptor generation failed.");
        }
        position+=ffi_tp->elements[i]->alignment;
    }
    return new_struct_descriptor(E, to_convert, fields);
}

void* to_ffi_type(Executor* E, Object o, ffi_type* ffi_tp, Object type_object, Object basic_types){
    switch(ffi_tp->type){
        case FFI_TYPE_STRUCT:{
            char* result=malloc(ffi_tp->size);
            CHECK_ALLOCATION(result)
            int position=0;
            for(int i=0; ffi_tp->elements[i]!=NULL; i++){
                Object field_object=get(E, type_object, to_int(i));
                Object field_name=get(E, field_object, to_int(0));
                Object field_type=get(E, field_object, to_int(1));
                Object field_value=get(E, o, field_name);
                int size=ffi_tp->elements[i]->size;
                void* converted_field=to_ffi_type(E, field_value, ffi_tp->elements[i], field_type, basic_types);
                memcpy(result+position, converted_field, size);
                free(converted_field);
                position+=ffi_tp->elements[i]->alignment;
            }
            return result;
        }
        case FFI_TYPE_POINTER:
            if(type_object.type==t_table) {
                Object first_field=get(E, type_object, to_int(0));
                if(first_field.type==t_string){
                    Object second_field=get(E, type_object, to_int(1));
                    ffi_type* sub_type=get_ffi_type_identifier(E, second_field, basic_types);
                    if(strcmp(first_field.text, "pointer")==0){
                        void** result=malloc(sizeof(void*));
                        if(is_struct_descriptor(E, o)){
                            // if passed argument is a struct descriptor use it's memory pointer
                            *result=struct_descriptor_get_pointer(E, o.tp);
                        } else {
                            // convert table to struct and pass the pointer to this struct
                            *result=to_ffi_type(E, o, sub_type, second_field, basic_types);
                        }
                        return result;
                    } else if(strcmp(first_field.text, "array")==0){
                        // convert sequential elements of the table to array elements
                        int count=count_array_part(E, o);
                        char* result=malloc(sub_type->size*count);
                        for(int i=0; i<count; i++){
                            memcpy(result+i*ffi_tp->size, to_ffi_type(E, o, sub_type, second_field, basic_types), sub_type->size);
                        }
                        return result;
                    }
                }
            } else if(EQUALS_STRING(type_object, "string")){
                char** casted=malloc(sizeof(char*));
                *casted=o.text;
                return casted;
            } else if(o.type==t_int) {
                void** casted=malloc(sizeof(void*));
                *casted=(void*)o.int_value;
                return casted;
            } else if(o.type==t_pointer) {
                void** casted=malloc(sizeof(void*));
                *casted=o.p;
                return casted;
            } 
            break;
        case FFI_TYPE_VOID:
            return NULL;
        default:
            if(o.type==t_int){
                switch(ffi_tp->type){
                    #define X(type, type_id, _) \
                    case type_id: \
                    { \
                        type* casted=malloc(sizeof(type)); \
                        CHECK_ALLOCATION(casted) \
                        *casted=o.int_value; \
                        return casted; \
                    }
                    
                    NUMERIC_TYPES
                    #undef X
                }
            } else if(o.type==t_float){
                switch(ffi_tp->type){
                #define X(type, type_id, _) \
                    case type_id: \
                    { \
                        type* casted=malloc(sizeof(type)); \
                        CHECK_ALLOCATION(casted) \
                        *casted=o.float_value; \
                        return casted; \
                    }
                    
                    NUMERIC_TYPES
                    #undef X
                }
            }
            break;
    }
    char* object_stringified=stringify(E, o);
    char* type_object_stringified=stringify(E, type_object);
    THROW_ERROR(WRONG_ARGUMENT_TYPE, "Couldn't convert %s to ffi type %s.", object_stringified, type_object_stringified);
    free(object_stringified);
    free(type_object_stringified);
    return NULL;
}

Object from_ffi_type(Executor* E, void* to_convert, ffi_type* ffi_tp, Object type_object, Object basic_types){
    switch(ffi_tp->type){
        // TODO converting to float
        #define X(type, type_id, _) \
        case type_id: \
            return to_int(*(type*)to_convert);
        NUMERIC_TYPES
        #undef X
        case FFI_TYPE_POINTER:
            if(type_object.type==t_string && strcmp(type_object.text, "string")==0){
                return to_string(*(char**)to_convert);
            } else if(type_object.type==t_table) {
                Object first_field=get(E, type_object, to_int(0));
                if(first_field.type==t_string){
                    Object second_field=get(E, type_object, to_int(1));
                    if(strcmp(first_field.text, "pointer")==0){
                        // here you will generate struct descriptor
                        // but for now just fall through to struct to table conversion
                    } else if(strcmp(first_field.text, "array")==0){
                        // here you will generate unbounded array accessor
                        // but for now just fall through to struct to table conversion
                    }
                    return to_struct_descriptor(E, *(void**)to_convert, get_ffi_type_identifier(E, second_field, basic_types), second_field);
                }
                break;
            } else {
                return to_pointer(*(void**)to_convert);
            }
        case FFI_TYPE_STRUCT:
        {
            Object o;
            table_init(E, &o);
            int position=0;
            for(int i=0; ffi_tp->elements[i]!=NULL; i++){
                void* field=(char*)to_convert+position;
                Object field_object=get(E, type_object, to_int(i));
                Object field_name=get(E, field_object, to_int(0));
                Object field_type=get(E, field_object, to_int(1));
                set(E, o, field_name, from_ffi_type(E, field, ffi_tp->elements[i], field_type, basic_types));
                int size=ffi_tp->elements[i]->size;
                position+=size;
            }
            return o;
        }
    }
    return null_const;
}

Object ffi_function_call(Executor* E, Object scope, Object* arguments, int arguments_count){
    //BOUND_FUNCTION_CHECK
    Object self=arguments[0];
    Object cif_pointer=get(E, self, to_string("cif"));
    REQUIRE_TYPE(cif_pointer, t_pointer);
    Object basic_types=get(E, self, to_string("basic_types"));

    Object function_pointer=get(E, self, to_string("function"));
    REQUIRE_TYPE(function_pointer, t_pointer);

    ffi_cif* cif=(ffi_cif*)cif_pointer.p;

    void** values=malloc(sizeof(void*)*(arguments_count-1));
    CHECK_ALLOCATION(values);
    for(int i=0; i<arguments_count-1; i++){
        Object type_object=get(E, self, to_int(i));
        values[i]=to_ffi_type(E, arguments[i+1], cif->arg_types[i], type_object, basic_types);
    }
    void* returned=malloc(cif->rtype->size);
    ffi_call(cif, FFI_FN(function_pointer.p), returned, values);

    Object result=from_ffi_type(E, returned, cif->rtype, get(E, self, to_int(arguments_count-1)), basic_types);

    for(int i=0; i<arguments_count-1; i++){
        free(values[i]);
    }

    free(values);
    free(returned);
    
    return result;
}

Object ffi_function_destroy(Executor* E, Object scope, Object* arguments, int arguments_count){
    //BOUND_FUNCTION_CHECK
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

Object ffi_function(Executor* E, Object scope, Object* arguments, int arguments_count){
    //BOUND_FUNCTION_CHECK
    Object self=arguments[0];
    REQUIRE_TYPE(self, t_table)
    Object function_name=arguments[1];
    REQUIRE_TYPE(function_name, t_string);
    Object basic_types=get(E, self, to_string("basic_types"));
    Object lib_pointer=get(E, self, to_int(0));
    REQUIRE_TYPE(lib_pointer, t_pointer)
 
    Object result;
    table_init(E, &result);

    char* find_symbol_error;
    void* function_address=find_symbol(lib_pointer.p, function_name.text, &find_symbol_error);
    if(function_address==NULL){
        dereference(E, &result);
        RETURN_ERROR("FFI_ERROR", function_name, find_symbol_error);
    }
    set(E, result, to_string("function"), to_pointer(function_address));

    int argument_types_count=arguments_count-3;
    ffi_type** argument_types=calloc(argument_types_count, sizeof(ffi_type*));
    ffi_type*  return_type=NULL;

    #define CONVERT(receiver, to_convert) \
        ffi_type* converted=get_ffi_type_identifier(E, to_convert, basic_types); \
        if(converted==NULL){ \
            dereference(E, &result); \
            USING_STRING(stringify(E, to_convert), \
                NEW_ERROR(result, "FFI_ERROR", to_convert, "Unknown type identifier %s.", str)) \
            goto error_cleanup; \
        } else { \
            (receiver)=converted; \
        }
    
    CHECK_ALLOCATION(argument_types)
    for(int i=0; i<argument_types_count; i++){
        CONVERT(argument_types[i], arguments[2+i])
        set(E, result, to_int(i), arguments[2+i]);
    }
    CONVERT(return_type, arguments[arguments_count-1]);
    set(E, result, to_int(argument_types_count), arguments[arguments_count-1]);
    
    ffi_cif* cif=malloc(sizeof(ffi_cif));
    CHECK_ALLOCATION(cif)
    
    ffi_status status = ffi_prep_cif(cif, FFI_DEFAULT_ABI, argument_types_count, return_type,
                                     argument_types);

    if (status != FFI_OK) {
        dereference(E, &result);
        NEW_ERROR(result, "FFI_ERROR", arguments[0], "ffi_prep_cif failed: %d\n", status);
        goto error_cleanup;
    }

    set(E, result, to_string("cif"), to_pointer(cif));
    set(E, result, to_string("basic_types"), basic_types);
    set(E, result, OVERRIDE(E, call), to_bound_function(E, result, 1, true, ffi_function_call));
    set(E, result, OVERRIDE(E, destroy), to_bound_function(E, result, 1, false, ffi_function_destroy));
    table_protect(result.tp);
    table_set(E, self.tp, function_name, result);

    return result;

    error_cleanup:
    for(int i=0; i<argument_types_count; i++){
        free(argument_types[i]);
    }
    free(argument_types);
    free(return_type);
    free(cif);
    return result;
}

Object ffi_struct_destroy(Executor* E, Object scope, Object* arguments, int arguments_count){
    //BOUND_FUNCTION_CHECK
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

Object ffi_struct_new_struct_descriptor(Executor* E, Object scope, Object* arguments, int arguments_count){
    //BOUND_FUNCTION_CHECK
    Object self=arguments[0];
    Object type_field=get(E, self, to_string("type"));
    REQUIRE_TYPE(type_field, t_pointer)
    ffi_type* type=type_field.p;

    // use libffi to fill alignment and size fields by creating a dummy function call interface returning the struct
    ffi_cif cif;
    ffi_status status = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 0, type, NULL);

    if (status != FFI_OK) {
        RETURN_ERROR("FFI_ERROR", arguments[0], "ffi_prep_cif failed: %d\n", status);
    }

    void* struct_pointer=malloc(type->size);
    return to_struct_descriptor(E, struct_pointer, type, self);
}

Object ffi_struct(Executor* E, Object scope, Object* arguments, int arguments_count){
    //BOUND_FUNCTION_CHECK
    Object self=arguments[0];
    Object basic_types=get(E, self, to_string("basic_types"));

    ffi_type* struct_type=malloc(sizeof(ffi_type));
    CHECK_ALLOCATION(struct_type)
    struct_type->size = struct_type->alignment = 0;
    struct_type->type = FFI_TYPE_STRUCT;
    struct_type->elements = calloc(arguments_count, sizeof(ffi_type*));
    CHECK_ALLOCATION(struct_type->elements)

    Object result;
    table_init(E, &result);
    set(E, result, to_string("type"), to_pointer(struct_type));

    for(int i=1; i<arguments_count; i++){
        // fields passed as arguments to this function are in format:
        // [field_name, field_type] 
        Object field_type=get(E, arguments[i], to_int(1));

        ffi_type* converted=get_ffi_type_identifier(E, field_type, basic_types);
        if(converted==NULL){
            dereference(E, &result);
            USING_STRING(stringify(E, field_type), \
                NEW_ERROR(result, "FFI_ERROR", field_type, "Unknown type identifier %s.", str))
            goto error_cleanup;
        } else {
            struct_type->elements[i-1]=converted;
        }
        set(E, result, to_int(i-1), arguments[i]);
    }
    struct_type->elements[arguments_count-1]=NULL;
    
    set(E, result, OVERRIDE(E, destroy), to_bound_function(E, result, 1, false, ffi_struct_destroy));
    set_function_bound(E, result, "new_struct_descriptor", 1, false, ffi_struct_new_struct_descriptor);
    table_protect(result.tp);
    return result;

    error_cleanup:
    for(int i=1; i<arguments_count; i++){
        free(struct_type->elements[i]);
    }
    free(struct_type->elements);
    free(struct_type);
    return result;
}

Object ffi_destroy(Executor* E, Object scope, Object* arguments, int arguments_count){
    //BOUND_FUNCTION_CHECK
    Object self=arguments[0];
    Object lib_pointer=get(E, self, to_int(0));
    REQUIRE_TYPE(lib_pointer, t_pointer);
    close_dll(lib_pointer.p);
    return null_const;
}

Object ffi_open(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object library_name_object=arguments[0];
    Object result;
    table_init(E, &result);

    char* library_name;
    if(library_name_object.type==t_string){
        library_name=library_name_object.text;
    } else if(library_name_object.type==t_null){
        library_name=NULL;
    } else {
        RETURN_ERROR("INCORRECT_ARGUMENT_TYPE", library_name_object, "Incorrect library name object passed to function ffi_open.")
    }
    char* get_dll_handle_error;
    void* dll_handle=get_dll_handle(library_name, &get_dll_handle_error);
    if(dll_handle==NULL){
        RETURN_ERROR("FFI_ERROR", library_name_object, get_dll_handle_error);
    }
    set(E, result, to_int(0), to_pointer(dll_handle));
    
    set_function_bound(E, result, "struct", 1, true, ffi_struct);
    set_function_bound(E, result, "function", 2, true, ffi_function);
    set(E, result, OVERRIDE(E, destroy), to_bound_function(E, result, 1, false, ffi_destroy));
    
    Object basic_types;
    table_init(E, &basic_types);
    #define X(type, type_id, type_variable) \
        set(E, basic_types, to_string(#type), to_pointer(&type_variable));
    NUMERIC_TYPES
    #undef X
    set(E, basic_types, to_string("string"), to_pointer(&ffi_type_pointer));
    set(E, basic_types, to_string("pointer"), to_pointer(&ffi_type_pointer));
    set(E, basic_types, to_string("void"), to_pointer(&ffi_type_void));
    set(E, result, to_string("basic_types"), basic_types);

    table_protect(result.tp);
    
    return result;
}

Object duck_module_init(Executor* E){
    Object module;
    table_init(E, &module);
    set_function(E, module, "open", 1, false, ffi_open);
    return module;
}

#undef NUMERIC_TYPES