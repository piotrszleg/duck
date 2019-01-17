#include <stdbool.h>
#include "repl.h"
#include "execution.h"

void repl(int use_bytecode);

int main(int argc, char *argv[]){
    if(argc>3){
        printf("Too many arguments.");
    } else {
        TRY_CATCH(
            bool use_bytecode=strcmp(argv[argc-1], "-ast")!=0;
            if(!use_bytecode){
                if(argc==3){
                    execute_file(argv[1], use_bytecode);
                } else {
                    repl(use_bytecode);
                }
            } else {
                if(argc>2){
                    printf("Flags should be placed after the filename.");
                } else if(argc==2){
                    execute_file(argv[1], use_bytecode);
                } else {
                    repl(use_bytecode);
                }
            }
        ,
            printf("Error occured:\n");
            printf(err_message);
            exit(-1);
        );
    }
}