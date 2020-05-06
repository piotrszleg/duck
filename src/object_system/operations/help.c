#include "help.h"

// TODO: loading help pages from files
Object help(Executor* E, Object subject){
    switch(subject.type){
        case t_table:
        {
            Object help_override=get(E, subject, OVERRIDE(E, help));
            if(help_override.type==t_string){
                printf("%s\n", help_override.text);
                return null_const;
            } else if(help_override.type!=t_null){
                Object result=call(E, help_override, &subject, 1);
                dereference(E, &help_override);
                return result;
            }
            break;
        }
        case t_function:
        {
            char* help_string=subject.fp->help;
            if(help_string!=NULL){
                printf("%s\n", help_string);
                return null_const;
            }
        }
        default:;
    }
    printf("No help available for this object.\n");
    return null_const;
}