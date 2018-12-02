#include "error_object.h"

object* new_error(char* type, char* message, object* cause){
    object* err=(object*)new_table();
    string* err_type=new_string();
    err_type->value=type;
    set(err, "type", err_type);
    string* err_message=new_string();
    err_message->value=message;
    set(err, "message", err_message);
    number* one=new_number();
    one->value=1;
    set(err, "is_error", one);
    set(err, "cause", cause);
    return err;
}