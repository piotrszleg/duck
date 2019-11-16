#include "stringification.h"

bool is_serializable(Object o) {
    switch(o.type) {
        case t_string:
        case t_float:
        case t_int:
        case t_null:
        case t_table:
            return true;
        default:
            return false;
    }
}

char* serialize(Executor* E, Object o) {
    if(o.type==t_table){
        Object serialize_override=get(E, o, OVERRIDE(E, serialize));
        if(serialize_override.type!=t_null){
            Object result=call(E, serialize_override, &o, 1);
            if(result.type!=t_string){
                return stringify_object(E, result);
            } else {
                return result.text;
            }
        } else {
            return table_serialize(E, o.tp);
        }
    } else if(is_serializable(o)){
        return stringify_object(E, o);
    } else {
        return strdup("");
    }
}

char* stringify(Executor* E, Object o){
    if(o.type==t_table){
        Object stringify_override=get(E, o, OVERRIDE(E, stringify));
        if(stringify_override.type!=t_null){
            Object result=call(E, stringify_override, &o, 1);
            if(result.type!=t_string){
                return stringify_object(E, result);
            } else {
                return result.text;
            }
        }
    }
    return stringify_object(E, o);
}

#define STRINGIFY_BUFFER_SIZE 16
// printf variant that returns pointer to formatted string
char* suprintf (const char * format, ...){
    char* buffer=malloc(STRINGIFY_BUFFER_SIZE*sizeof(char));
    CHECK_ALLOCATION(buffer);
    int buffer_size=STRINGIFY_BUFFER_SIZE;
    va_list args;

    while(true){
        va_start (args, format);
        int vsprintf_result=vsnprintf (buffer, buffer_size, format, args);
        va_end (args);
        if(vsprintf_result>=buffer_size){
            buffer_size*=2;
            buffer=realloc(buffer, buffer_size*sizeof(char));
            CHECK_ALLOCATION(buffer);
        } else {
            break;
        }
    }
    return buffer;
}

char* quote_string(char* original) {
    ReplacementPair replacements[]={
        CONSTANT_REPLACEMENT_PAIR("\"", "\\\""),
        CONSTANT_REPLACEMENT_PAIR("\t", "\\t"),
        CONSTANT_REPLACEMENT_PAIR("\n", "\\n"),
        CONSTANT_REPLACEMENT_PAIR("\\", "\\\\")
    };
    // escape quotes
    char* replacement_result=string_replace_multiple(original, replacements, 4);
    char* text;
    char* result;
    if(replacement_result!=NULL){
        text=replacement_result;
    } else {
        text=original;
    }
    if(is_valid_name(text)){
        int length=strlen(text)+2;
        result=malloc(length*sizeof(char));
        snprintf(result, length, "'%s", text);
    } else {
        int length=strlen(text)+3;
        result=malloc(length*sizeof(char));
        snprintf(result, length, "\"%s\"", text);
    }

    if(replacement_result!=NULL){
        free(replacement_result);
    }
    return result;
}

char* stringify_object(Executor* E, Object o){
    switch(o.type){
        case t_string:
        {
            return quote_string(o.text);
        }
        case t_int:
            return suprintf("%d", o.int_value);
        case t_float:
            return suprintf("%f", o.float_value);
        case t_table:
            return table_stringify(E, o.tp);
        case t_function:
        {
            Function* f=o.fp;
            if(f->argument_names!=NULL){
                char* buffer=malloc(STRINGIFY_BUFFER_SIZE*sizeof(char));
                CHECK_ALLOCATION(buffer);
                buffer[0]='\0';
                int buffer_size=STRINGIFY_BUFFER_SIZE;
                int buffer_count=0;// how many characters were written to the buffer
                
                // if buffer isn't big enough to hold the added string characters double its size
                #define BUFFER_WRITE(string, count) \
                    while(buffer_size<=buffer_count+count){ \
                        buffer_size*=2; \
                        buffer=realloc(buffer, buffer_size*sizeof(char)); \
                    } \
                    strncat(buffer, string, buffer_size); \
                    buffer_count+=count;
                
                BUFFER_WRITE("<function (", 10);
                int first=1;
                for (int i = 0; i < f->arguments_count; i++){
                    char* argument_name=f->argument_names[i];
                    int character_count=strlen(argument_name);

                    if(first){
                        BUFFER_WRITE(argument_name, character_count);
                        first=0;
                    } else {
                        int formatted_count=3+character_count;
                        char* argument_buffer=malloc(formatted_count*sizeof(char));
                        snprintf(argument_buffer, formatted_count, ", %s", argument_name);
                        BUFFER_WRITE(argument_buffer, formatted_count);
                        free(argument_buffer);
                    }
                }
                if(f->variadic){
                    BUFFER_WRITE("...", 3);
                }
                BUFFER_WRITE(")>\0", 2);
                
                return buffer;
            } else {
                char* buffer=malloc(16*sizeof(char*));
                if(f->variadic){
                    snprintf(buffer, 16, "<function %i...>", f->arguments_count);
                } else {
                    snprintf(buffer, 16, "<function %i>", f->arguments_count);
                }
                return buffer;
            }
        }
        case t_coroutine:
            return suprintf("<coroutine %#x>", (long unsigned)o.co);
        case t_pointer:
            return suprintf("<pointer %#x>", (long unsigned)o.p);
        case t_managed_pointer:
            return suprintf("<managed_pointer %#x>", (long unsigned)o.mp);
        case t_symbol: {
            char* quoted_comment=quote_string(o.sp->comment);
            char* result=suprintf("<symbol %s>", quoted_comment);
            free(quoted_comment);
            return result;
        }
        case t_null:
            return strdup("null");
        default:
            return strdup("<INCORRECT_OBJECT_POINTER>");
    }
}
#undef STRINGIFY_BUFFER_SIZE