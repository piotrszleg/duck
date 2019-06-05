#include "import_dll.h"

void* get_dll_handle(char* module_name){
    #ifdef WINDOWS
    HINSTANCE lib_handle = LoadLibrary(module_name);

    if(lib_handle != NULL) {
        return lib_handle;
    } else{
        // return error object with message
        LPVOID lpMsgBuf;
        DWORD dw = GetLastError(); 
        FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );
        // TODO: find a way to return the error
        //RETURN_ERROR("DLL_IMPORT_ERROR", null_const, (char*)lpMsgBuf)
        return NULL;
    }
    #else
    void* lib_handle = dlopen(module_name, RTLD_LAZY);
    if (!lib_handle) {
        dlclose(lib_handle);
        //RETURN_ERROR("DLL_IMPORT_ERROR", null_const, dlerror())
	}
    ModuleInitFunction init_function = (ModuleInitFunction)dlsym(lib_handle, "duck_module_init");
    char *error;
    if ((error = dlerror()) != NULL) {
        dlclose(lib_handle);
	    //RETURN_ERROR("DLL_IMPORT_ERROR", null_const, error)
	}
    return lib_handle;
    #endif
}
void* find_symbol(void* dll_handle, char* name){
    #ifdef WINDOWS
    return GetProcAddress(dll_handle, name);
    #else
    void* result=dlsym(lib_handle, "duck_module_init");
    char *error;
    if ((error = dlerror()) != NULL) {
	    //RETURN_ERROR("DLL_IMPORT_ERROR", null_const, error)
	}
    return result;
    #endif
}   

void close_dll(void* dll_handle){
    #ifdef WINDOWS
    FreeLibrary(dll_handle);
    #else
    dlclose(lib_handle);
    #endif
}

Object import_dll(Executor* E, const char* module_name){
    typedef Object (*ModuleInitFunction) (Executor*);
    Object result;
    #ifdef WINDOWS
    HINSTANCE lib_handle = LoadLibrary(module_name);

    if(lib_handle != NULL) {
        ModuleInitFunction init_function = (ModuleInitFunction)GetProcAddress(lib_handle, "duck_module_init");   
        
        if (init_function != NULL){
            result=init_function(E);
        } else {
            result=null_const;
        }
        // TODO: mechanism of counting references to the module
        //FreeLibrary(lib_handle);
    } else{
        // return error object with message
        LPVOID lpMsgBuf;
        DWORD dw = GetLastError(); 
        FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );
        RETURN_ERROR("DLL_IMPORT_ERROR", null_const, (char*)lpMsgBuf)
    }
    #else
    void* lib_handle = dlopen(module_name, RTLD_LAZY);
    if (!lib_handle) {
        dlclose(lib_handle);
        RETURN_ERROR("DLL_IMPORT_ERROR", null_const, dlerror())
	}
    ModuleInitFunction init_function = (ModuleInitFunction)dlsym(lib_handle, "duck_module_init");
    char *error;
    if ((error = dlerror()) != NULL) {
        dlclose(lib_handle);
	    RETURN_ERROR("DLL_IMPORT_ERROR", null_const, error)
	}
    if(init_function!=NULL){
        result=init_function();
    } else {
        result=null_const;
    }
    //dlclose(lib_handle);
    #endif
    return result;
}