#include <stdbool.h>
#include "object_system/object.h"
#include "options.h"

int main(int argc, char *argv[]){
    TRY_CATCH(
        table_init(&patching_table);
        reference(&patching_table);

        handle_arguments(argc, argv);
        
        dereference(&patching_table);
        gc_run(NULL, 0);
    ,
        printf("Error occured:\n");
        printf(err_message);
        exit(-1);
    );
}