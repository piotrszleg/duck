#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "repl.h"
#include "execution.h"

static const char* VERSION="0.0.1";
static const char* ARGUMENTS_HELP= \
    "\nDuck 0.0.1\n"
    "\nOptions without argument are:\n"
    "\t-version\n"
    "\t-help\n"
    "\t-?\n"
    "\nOptions with integer argument are:\n"
    #define BOOL(name, default)
    #define UINT(name, default) \
        "\t-"#name"=<number>\n"
    OPTIONS
    #undef BOOL
    #undef UINT
    "Example of usage: \"-collected_calls=10\"\n"
    "\nBOOL options are:\n"
    #define BOOL(name, default) \
        "\t-"#name"\n"
    #define UINT(name, default)
    OPTIONS
    #undef BOOL
    #undef UINT
    "To disable them prepend their name with \"dont_\", for example: \"-dont_print_ast\"\n"
    "You can also enable and disable them using equality sign, for example: \"-print_ast=1\" and \"-print_ast=0\"\n"
    "\nFirst argument without \"-\" is script file name, the arguments following it are passed to the script in variable named \"arguments\"\n"
    "If script file isn't provided a read-eval-print loop is started.\n\n"
    ;

const Options default_options={
    .file_path=NULL,
    .script_arguments=NULL,
    .should_run=true,
    .repl=false,
    #define BOOL(name, default) .name=default,
    #define UINT(name, default) .name=default,
    OPTIONS
    #undef BOOL
    #undef UINT
};

void read_arguments(Options* options, int arguments_count, char **arguments) {
    int i=0;
    
    for(; i<arguments_count; i++){
        if(arguments[i][0]=='-') {
            bool matched=false;
            #define OPTION(name, action) \
                if(strcmp(arguments[i]+1, name)==0){ \
                    matched=true; \
                    {action} \
                }
            #define BOOL(name, default) \
                OPTION(#name, options->name=true;) \
                OPTION(#name"=1", options->name=true;) \
                OPTION("dont_"#name, options->name=false;) \
                OPTION(#name"=0", options->name=false;)
            #define UINT(name, default) \
                if(strings_counted_equal(arguments[i]+1, #name"=", sizeof(#name"=")-1)){ \
                    matched=true; \
                    options->name=strtol(arguments[i]+1+sizeof(#name"=")-1, NULL, 10); \
                }
            OPTIONS
            #undef BOOL
            #undef UINT
            OPTION("repl", options->repl=true; ) \
            OPTION("version", printf("%s", VERSION); options->should_run=false; )
            OPTION("?", printf("%s", ARGUMENTS_HELP);
                    options->should_run=false; )
            OPTION("help", printf("%s", ARGUMENTS_HELP);
                    options->should_run=false; )
            #undef OPTION
            #undef BOOL_OPTION
            if(!matched){
                printf("Unknown command %s", arguments[i]);
                options->should_run=false;
                return;
            }
        } else {
            if(options->file_path!=NULL){
                free(options->file_path);
            }
            options->file_path=strdup(arguments[i]);
            i++;
            break;// arguments after filename are script arguments
        }
    }
    int additional_arguments_count=arguments_count-i;
    if(options->script_arguments!=NULL) {
        if(additional_arguments_count>0){
            int scripts_arguments_count=0;
            for(; options->script_arguments[scripts_arguments_count]!=NULL; scripts_arguments_count++);
            options->script_arguments=realloc(options->script_arguments, sizeof(char*)*(scripts_arguments_count+additional_arguments_count+1));// +1 for NULL teminator
            CHECK_ALLOCATION(options->script_arguments)
            for(int j=0; j<additional_arguments_count; j++){
                options->script_arguments[scripts_arguments_count+j]=strdup(arguments[i+j]);
            }
            options->script_arguments[scripts_arguments_count+additional_arguments_count]=NULL;
        }
    } else {
        options->script_arguments=malloc(sizeof(char*)*(additional_arguments_count+1));// +1 for NULL teminator
        CHECK_ALLOCATION(options->script_arguments)
        for(int j=0; j<additional_arguments_count; j++){
            options->script_arguments[j]=strdup(arguments[i+j]);
        }
        options->script_arguments[additional_arguments_count]=NULL;
    }
}

void options_deinit(Options* options){
    free(options->file_path);
    if(options->script_arguments!=NULL){
        for(int i=0; options->script_arguments[i]!=NULL; i++){
            free(options->script_arguments[i]);
        }
        free(options->script_arguments);
    }
}

void read_arguments_from_file(Options* options){
    FILE* fp=fopen("default_arguments.txt", "r");
    if(fp!=NULL){
        char* content=read_entire_file(fp);
        if(content==NULL){
            fclose(fp);
            return;
        }
        int arguments_size=8;
        char** arguments=malloc(sizeof(char*)*arguments_size);
        CHECK_ALLOCATION(arguments)
        int arguments_count=0;
        int start=0;
        int i=0;
        bool repeat=true;
        while(repeat){
            if(content[i]=='\0'){
                repeat=false;
            }
            if(content[i]==' ' || content[i]=='\n' || content[i]=='\0') {
                if(i!=start){// handling repeated whitespace and whitespace at the begginging
                    content[i]='\0';
                    arguments[arguments_count]=&content[start];
                    arguments_count++;
                    if(arguments_count>=arguments_size){
                        arguments_size*=2;
                        arguments=realloc(arguments, sizeof(char*)*arguments_size);
                    }
                }
                start=i+1;
            }
            i++;
        }
        read_arguments(options, arguments_count, arguments);
        free(arguments);
        free(content);
        fclose(fp);
    }
}

void handle_arguments(int argc, char **argv) {
    Options options=default_options;
    read_arguments_from_file(&options);
    read_arguments(&options, argc-1, argv+1);// skip executable name

    if(options.should_run){
        Executor E;
        executor_init(&E);
        E.options=options;
        TRY_CATCH(
            if(options.repl || options.file_path==NULL){
                E.file="repl";
                repl(&E);
            } else {
                execute_file(&E, options.file_path);
            }
        ,
            printf("Error occurred:\n");
            printf("%s", err_message);
            exit(-1);
        );
        executor_deinit(&E);
    }
    options_deinit(&options);
}