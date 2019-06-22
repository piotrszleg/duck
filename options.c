#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "repl.h"
#include "execution.h"

static const char* version="0.0.1";

const Options default_options={
    .ast_only=false,
    .print_ast=false,
    .print_ast_optimisations=false,
    .print_bytecode=false,
    .print_bytecode_optimisations=false,
    .optimise_ast=true,
    .optimise_bytecode=true,
    .debug_mode=false
};

void handle_arguments(int argc, char **argv) {
    Options options=default_options;
    #define OPTIONS \
        X("ast_only", options.ast_only=true;) \
        X("print_ast", options.print_ast=true;) \
        X("print_ast_optimisations", options.print_ast_optimisations=true;) \
        X("print_bytecode", options.print_bytecode=true;) \
        X("print_bytecode_optimisations", options.print_bytecode_optimisations=true;) \
        X("disable_ast_optimisations", options.optimise_ast=false;) \
        X("disable_bytecode_optimisations", options.optimise_bytecode=false;) \
        X("debug", options.debug_mode=true;) \
        X("version", printf(version); exit(0); )\
        X("?", printf("duck %s\nAllowed options are: \n-version\n-ast_only\n-disable_ast_optimisations\n-disable_bytecode_optimisations\n-debug\n-?\n" \
                       "You can either provide a file path or a read-eval-print loop is started.", version); \
                return;)

    char* file_path=NULL;
    
    for(int i=1; i<argc; i++){
        if(argv[i][0]=='-') {
            bool matched=false;
            #define X(option_name, action) \
                if(strcmp(argv[i]+1, option_name)==0){ \
                    matched=true; \
                    {action} \
                }
            OPTIONS
            #undef X
            if(!matched){
                printf("Unknown command %s", argv[i]);
                return;
            }
        } else {
            if(file_path==NULL){
                file_path=argv[i];
            } else {
                printf("Only one file can be given as input.");
                return;
            }
        }
    }

    Executor E;
    E.gc=malloc(sizeof(GarbageCollector));
    E.options=options;
    E.ast_execution_state.returning=false;
    vector_init(&E.ast_execution_state.used_objects, sizeof(Object), 8);
    object_system_init(&E);
    bytecode_environment_init(&E.bytecode_environment);

    TRY_CATCH(
        if(file_path!=NULL){   
            execute_file(&E, file_path);
        } else {
            repl();
        }
    ,
        printf("Error occured:\n");
        printf(err_message);
        exit(-1);
    );
    object_system_deinit(&E);
    free(E.gc);
}