#ifndef IMPORT_DLL_H
#define IMPORT_DLL_H

#define WINDOWS

#ifdef WINDOWS
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "../object_system/object.h"

object import_dll(const char*);
void* get_dll_handle(char* module_name);
void* find_symbol(void* dll_handle, char* name);
void close_dll(void* dll_handle);

#endif