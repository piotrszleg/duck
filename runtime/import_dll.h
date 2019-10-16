#ifndef IMPORT_DLL_H
#define IMPORT_DLL_H

// #define WINDOWS

#ifdef WINDOWS
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "../object_system/object.h"

Object import_dll(Executor* E, const char* library_name);
void* get_dll_handle(char* library_name, char** error);
void* find_symbol(void* dll_handle, char* name, char** error);
void close_dll(void* dll_handle);

#endif