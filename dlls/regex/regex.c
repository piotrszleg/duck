#include "../../object_system/object.h"
#include "../../datatypes/map.h"
#include "regex.h"

Object regex_object_destroy(Executor* E, Object scope, Object* arguments, int arguments_count){
    BOUND_FUNCTION_CHECK
    Object self=arguments[0];
    Object compiled_regex=get(E, self, to_int(0));
    REQUIRE_TYPE(compiled_regex, t_pointer);

    regfree((regex_t*)compiled_regex.p);
    free(compiled_regex.p);

    return null_const;
}

Object regex_object_call(Executor* E, Object scope, Object* arguments, int arguments_count){
    BOUND_FUNCTION_CHECK
    Object self=arguments[0];
    Object compiled_regex=get(E, self, to_int(0));
    REQUIRE_TYPE(compiled_regex, t_pointer);

    Object str=arguments[1];
    REQUIRE_ARGUMENT_TYPE(str, t_string);

    const int matches_count=32;
    regmatch_t matches[32];
    Object result;
    table_init(E, &result);
    if(!regexec((regex_t*)compiled_regex.p, str.text, matches_count, matches, 0)){ 
        for(int i=0; i<matches_count; i++){
            if(matches[i].rm_so<0){
                break;
            }
            Object match_object;
            table_init(E, &match_object);
            set(E, match_object, to_string("start"), to_int(matches[i].rm_so));
            set(E, match_object, to_string("end"), to_int(matches[i].rm_eo));
            set(E, result, to_int(i), match_object);
        }
    }
    return result;
}

char* substring(char* original, int start, int end){
    int substring_size=end-start+1;
    char* substring=malloc(substring_size);
    memcpy(substring, original+start, substring_size-1);
    substring[substring_size-1]='\0';
    return substring;
}

Object regex_object_split(Executor* E, Object scope, Object* arguments, int arguments_count){
    BOUND_FUNCTION_CHECK
    Object self=arguments[0];
    Object compiled_regex=get(E, self, to_int(0));
    REQUIRE_TYPE(compiled_regex, t_pointer);

    Object str=arguments[1];
    REQUIRE_ARGUMENT_TYPE(str, t_string);

    const int matches_count=32;
    regmatch_t matches[32];
    Object result;
    table_init(E, &result);
    if(!regexec((regex_t*)compiled_regex.p, str.text, matches_count, matches, 0)){ 
        for(int i=0; i<matches_count; i++){
            if(matches[i].rm_so<0){
                break;
            }
            char* substr=substring(str.text, matches[i].rm_so, matches[i].rm_eo);
            set(E, result, to_int(i), to_string(substr));
        }
    }
    return result;
}

Object regex_object_replace(Executor* E, Object scope, Object* arguments, int arguments_count){
    BOUND_FUNCTION_CHECK
    Object self=arguments[0];
    Object compiled_regex=get(E, self, to_int(0));
    REQUIRE_TYPE(compiled_regex, t_pointer);

    Object str=arguments[1];
    REQUIRE_ARGUMENT_TYPE(str, t_string);
    Object replacement_string_object=arguments[2];
    REQUIRE_ARGUMENT_TYPE(replacement_string_object, t_string);
    char* replacement_string=replacement_string_object.text;
    size_t replacement_string_length=strlen(replacement_string);

    stream s;
    stream_init(&s, replacement_string_length);

    const int matches_count=32;
    regmatch_t matches[32];
    if(!regexec((regex_t*)compiled_regex.p, str.text, matches_count, matches, 0)){ 
        for(char* p=replacement_string; *p!='\0'; p++){
            if(*p=='$' && *(p+1)!='$'){
                switch (*(p+1)) {
                case '$':{
                    stream_push_const_string(&s, "$");
                    p++;
                    break;
                }
                case '&':{
                    int match_length=matches[0].rm_eo-matches[0].rm_so;
                    stream_push(&s, str.text+matches[0].rm_so, match_length);
                    p++;
                    break;
                }
                case '`':{
                    int match_length=matches[0].rm_so;
                    stream_push(&s, str.text, match_length);
                    p++;
                    break;
                }
                case '\'':{
                    int match_length=strlen(str.text)-matches[0].rm_eo;
                    stream_push(&s, str.text+matches[0].rm_eo, match_length);
                    p++;
                    break;
                }
                default:{
                    char* after_number=p;
                    int parsed_number=strtol(p+1, &after_number, 10);
                    if(parsed_number>0){
                        int match_length=matches[parsed_number].rm_eo-matches[parsed_number].rm_so;
                        stream_push(&s, str.text+matches[parsed_number].rm_so, match_length);
                        p=after_number-1;
                    } else {
                        stream_push(&s, p, 1);
                    }
                }
                }
            } else {
                stream_push(&s, p, 1);
            }
        }
    }
    stream_push_const_string(&s, "\0");
    return to_string(stream_get_data(&s));
}

Object regex_object_process(Executor* E, Object scope, Object* arguments, int arguments_count){
    BOUND_FUNCTION_CHECK
    Object self=arguments[0];
    Object compiled_regex=get(E, self, to_int(0));
    REQUIRE_TYPE(compiled_regex, t_pointer);

    Object str=arguments[1];
    REQUIRE_ARGUMENT_TYPE(str, t_string);

    Object processing_function=arguments[2];

    const int matches_count=32;
    regmatch_t matches[32];

    if(!regexec((regex_t*)compiled_regex.p, str.text, matches_count, matches, 0)){
        int processor_arguments_count;
        for(processor_arguments_count=0; processor_arguments_count<matches_count; processor_arguments_count++){
            if(matches[processor_arguments_count].rm_so<0){
                break;
            }
        }
        processor_arguments_count+=2;
        Object* processor_arguments=malloc(sizeof(Object)*processor_arguments_count);
        for(int i=0; i<matches_count; i++){
            if(matches[i].rm_so<0){
                break;
            }
            processor_arguments[i]=to_string(substring(str.text, matches[i].rm_so, matches[i].rm_eo));
        }
        processor_arguments[processor_arguments_count-2]=to_int(matches[0].rm_so);
        processor_arguments[processor_arguments_count-1]=str;
        return call(E, processing_function, processor_arguments, processor_arguments_count);
    } else {
        return str;
    }
}

Object regex_module_compile(Executor* E, Object scope, Object* arguments, int arguments_count){
    Object pattern=arguments[1];
    REQUIRE_ARGUMENT_TYPE(pattern, t_string);

    int flags=REG_EXTENDED;
    for(int a=2; a<arguments_count; a++){
        if(arguments[a].type!=t_string){
            RETURN_ERROR("INCORRECT_ARGUMENT_TYPE", arguments[a], "Regex flags should be of type string.")
        }
        if(strcmp(arguments[a].text, "newline_divides")==0){
            flags|=REG_NEWLINE;
        } else if(strcmp(arguments[a].text, "ignore_case")==0){
            flags|=REG_ICASE;
        } else if(strcmp(arguments[a].text, "basic")==0){
            flags&=~REG_EXTENDED;// remove flag REG_EXTENDED
        } else {
            RETURN_ERROR("REGEX_ERROR", arguments[a], "Unknown flag.")
        }
    }
    
    Object result;
    table_init(E, &result);
    regex_t* compiled_regex=malloc(sizeof(regex_t));
    int regcomp_result=regcomp(compiled_regex, pattern.text, flags);
    if(regcomp_result!=0) {
        regfree((regex_t*)compiled_regex);
        free(compiled_regex);
        size_t length = regerror (regcomp_result, compiled_regex, NULL, 0);
        char* buffer = malloc (length);
        regerror (regcomp_result, compiled_regex, buffer, length);
        RETURN_ERROR("REGEX_ERROR", arguments[0], buffer);
    } else {
        set(E, result, OVERRIDE(E, call), to_bound_function(E, result, 1, true, regex_object_call));
        set_function_bound(E, result, "split", 2, false, regex_object_split);
        set_function_bound(E, result, "replace", 3, false, regex_object_replace);
        set_function_bound(E, result, "process", 3, false, regex_object_process);
        set(E, result, OVERRIDE(E, destroy), to_bound_function(E, result, 1, false, regex_object_destroy));
        table_protect(result.tp);
        return result;
    }
}

Object duck_module_init(Executor* E){
    Object module;
    table_init(E, &module);
    set_function(E, module, "compile", 2, true, regex_module_compile);
    return module;
}