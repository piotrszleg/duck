#ifndef STRINGIFICATION_H
#define STRINGIFICATION_H

#include "../object.h"

char* suprintf (const char * format, ...);
char* stringify_object(Executor* E, Object o);
char* stringify(Executor* E, Object o);

bool is_serializable(Object o);
char* serialize(Executor* E, Object o);

#endif