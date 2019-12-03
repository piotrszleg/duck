#include "debugger.h"

void debugger_init(Debugger* debugger){
    vector_init(&debugger->breakpoints, sizeof(Breakpoint*), 4);
    debugger->running=false;
}

void debugger_deinit(Debugger* debugger){
    for(int i=0; i<vector_count(&debugger->breakpoints); i++){
        free(pointers_vector_get(&debugger->breakpoints, i));
    }
    vector_deinit(&debugger->breakpoints);
}

void debugger_update(Executor* E, Debugger* debugger){}
/*
void debugger_update(Executor* E, Debugger* debugger){
    if(debugger->running){
        for(int i=0; i<vector_count(&debugger->breakpoints); i++){
            Breakpoint* br=(Breakpoint*)pointers_vector_get(&debugger->breakpoints, i);

            if(strcmp(E->file, br->file)==0
            && E->line==br->line) {
                debugger->running=false;
            }
        }
        if(debugger->running){
            return;
        }
    }
    char input[128];
    while(true){
        printf(">>");
        if(fgets_no_newline(input, sizeof(input), stdin)==NULL){
            return;
        }
        #define COMMAND(name, body) if(strcmp(input, name)==0){body return;}
        #define COMMAND_PARAMETERIZED(name, body) \
            if(strstr(input, name)==input){ \
                char* parameter=input+sizeof(name); \
                body \
                return; \
            }
        COMMAND("next",)
        COMMAND("",)
        COMMAND("run", debugger->running=true;)
        COMMAND("help",
            printf("available commands are: \nnext \nrun \nposition \nmemory \nstack \nscope " \
            "\neval <expression> \nbreakpoints \nbreak <file>:<line> \nremove <file>:<line>\n");
        )
        COMMAND("position",
            char e_info[128];
            get_execution_info(E, e_info, sizeof(e_info));
            printf("%s\n", e_info);
        )
        COMMAND("memory",
            print_allocated_objects(E);
        )
        COMMAND("stack",
            print_object_stack(E, &E->stack);
        )
        COMMAND("scope",
            USING_STRING(stringify(E, E->scope),
                printf("%s\n", str));
        )
        COMMAND_PARAMETERIZED("eval", 
            Object result=evaluate_string(E, parameter, E->scope);
            USING_STRING(stringify(E, result),
                printf("%s\n", str));
            dereference(E, &result);
            return;
        )
        COMMAND("breakpoints", 
            for(int i=0; i<vector_count(&debugger->breakpoints); i++){
                Breakpoint* br=(Breakpoint*)pointers_vector_get(&debugger->breakpoints, i);
                printf("%s:%i\n", br->file, br->line);
            }
        )
        COMMAND_PARAMETERIZED("break",
            Breakpoint* b=malloc(sizeof(Breakpoint));
            CHECK_ALLOCATION(b)

            // parameter has syntax file:line_number
            int i=0;
            while(parameter[i]!=':' && parameter[i]!='\0') i++;
            if(parameter[i]=='\0'){
                printf("Wrong parameter given to break command: %s", parameter);
            }

            char* buf=malloc((i+1)*(sizeof(char)));
            memcpy(buf, parameter, i);
            buf[i]='\0';

            b->file=buf;
            b->line=atoi(parameter+i+1);
            
            vector_push(&debugger->breakpoints, &b);
            return;
        )
        COMMAND_PARAMETERIZED("remove",
            Breakpoint b;

            int i=0;
            while(parameter[i]!=':' && parameter[i]!='\0') i++;
            if(parameter[i]=='\0'){
                printf("Wrong parameter given to remove command: %s", parameter);
            }

            char* buf=malloc((i+1)*(sizeof(char)));
            memcpy(buf, parameter, i);
            buf[i]='\0';

            b.file=buf;
            b.line=atoi(parameter+i+1);

            for(int i=0; i<vector_count(&debugger->breakpoints); i++){
                Breakpoint* br=(Breakpoint*)pointers_vector_get(&debugger->breakpoints, i);
                if(strcmp(br->file, b.file)==0 && br->line==b.line){
                    vector_delete(&debugger->breakpoints, i);
                }
            }
            free(buf);
            return;
        )
        #undef COMMAND
        #undef COMMAND_PARAMETERIZED
        printf("Unknown command \"%s\"\n", input);
    }
}
*/