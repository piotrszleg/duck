#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "repl.h"
#include "execution.h"

static const char* version="0.0.1";

const options default_options={
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
    options opt=default_options;
    #define OPTIONS \
        X("ast_only", opt.ast_only=true;) \
        X("print_ast", opt.print_ast=true;) \
        X("print_ast_optimisations", opt.print_ast_optimisations=true;) \
        X("print_bytecode", opt.print_bytecode=true;) \
        X("print_bytecode_optimisations", opt.print_bytecode_optimisations=true;) \
        X("disable_ast_optimisations", opt.optimise_ast=false;) \
        X("disable_bytecode_optimisations", opt.optimise_bytecode=false;) \
        X("debug", opt.debug_mode=true;) \
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

    executor Ex;
    Ex.opt=opt;
    
    object_system_init();
    TRY_CATCH(
        if(file_path!=NULL){
            
            execute_file(&Ex, file_path);
        } else {
            repl();
        }
    ,
        printf("Error occured:\n");
        printf(err_message);
        exit(-1);
    );
    object_system_deinit(&Ex);
}