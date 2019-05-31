#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "repl.h"
#include "execution.h"

bool g_ast_only=false;
bool g_print_ast=false;
bool g_print_ast_optimisations=false;
bool g_print_bytecode=false;
bool g_print_bytecode_optimisations=false;
bool g_optimise_ast=true;
bool g_optimise_bytecode=true;
bool g_debug_mode=false;

static const char* version="0.0.1";

void handle_arguments(int argc, char **argv) {
    #define OPTIONS \
        X("ast_only", g_ast_only=true;) \
        X("print_ast", g_print_ast=true;) \
        X("print_ast_optimisations", g_print_ast_optimisations=true;) \
        X("print_bytecode", g_print_bytecode=true;) \
        X("print_bytecode_optimisations", g_print_bytecode_optimisations=true;) \
        X("disable_ast_optimisations", g_optimise_ast=false;) \
        X("disable_bytecode_optimisations", g_optimise_bytecode=false;) \
        X("debug", g_debug_mode=true;) \
        X("version", printf(version); exit(0); )\
        X("?", printf("duck %s\nAllowed options are: \n-version\n-ast_only\n-disable_ast_optimisations\n-disable_bytecode_optimisations\n-debug\n-?\n" \
                       "You can either provide a file path or a read-eval-print loop is started.", version); \
                exit(0);)

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
                exit(-1);
            }
        } else {
            if(file_path==NULL){
                file_path=argv[i];
            } else {
                printf("Only one file can be given as input.");
                exit(-2);
            }
        }
    }
    if(file_path!=NULL){
        execute_file(file_path);
    } else {
        repl();
    }
}