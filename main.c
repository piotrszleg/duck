#include <stdbool.h>
#include "object_system/object.h"
#include "options.h"

int main(int argc, char *argv[]){
    object_system_init();
    TRY_CATCH(
        handle_arguments(argc, argv);
    ,
        printf("Error occured:\n");
        printf(err_message);
        exit(-1);
    );
    object_system_deinit();
}