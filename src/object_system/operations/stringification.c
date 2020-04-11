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
            stream s;
            stream_init(&s, STRINGIFY_BUFFER_SIZE);
            stream_push_const_string(&s, "<");
            if(f->name!=NULL){
                stream_push_string(&s, f->name);
            }
            if(f->argument_names!=NULL){
                
                stream_push_const_string(&s, "(");
                bool first=false;
                for (int i = 0; i < f->arguments_count; i++){
                    char* argument_name=f->argument_names[i];
                    if(!first){
                        stream_push_const_string(&s, ", ");
                    }
                    stream_push_string(&s, argument_name);
                    first=false;
                }
                if(f->variadic){
                    stream_push_const_string(&s, "...");
                }
                stream_push_const_string(&s, ")");
            } else {
                char* buffer=malloc(8*sizeof(char*));
                if(f->variadic){
                    snprintf(buffer, 8, " %i", f->arguments_count);
                } else {
                    snprintf(buffer, 8, " %i...", f->arguments_count);
                }
                stream_push_string(&s, buffer);
                free(buffer);
            }
            stream_push_const_string(&s, ">\0");
            return stream_get_data(&s);
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