#include "import_dll.h"

#ifdef WINDOWS
char* get_windows_error_message() {
    LPVOID buffer;
    DWORD dw = GetLastError(); 
    FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
    FORMAT_MESSAGE_FROM_SYSTEM |
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    dw,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPTSTR) &buffer,
    0, NULL );
    return (char*)buffer;
}
#endif

void* get_dll_handle(char* library_name, char** error){
    #ifdef WINDOWS
    HINSTANCE lib_handle = LoadLibrary(library_name);

    if(lib_handle == NULL) {
        *error=strdup(get_windows_error_message());
        return NULL;
    } else {
        return lib_handle;
    }
    #else
    void* lib_handle = dlopen(library_name, RTLD_LAZY);
    if (lib_handle==NULL) {
        dlclose(lib_handle);
        *error=strdup(dlerror());
        return NULL;
	} else {
        return lib_handle;
    }
    #endif
}
void* find_symbol(void* dll_handle, char* name, char** error){
    #ifdef WINDOWS
    FARPROC address=GetProcAddress(dll_handle, name);
    if(address==NULL){
        *error=strdup(get_windows_error_message());
        return NULL;
    } else{
        return address;
    }
    #else
    void* address=dlsym(dll_handle, name);
    if ((*error = dlerror()) != NULL) {
        *error=strdup(*error);
        return NULL;
	} else {
        return address;
    }
    #endif
}   

void close_dll(void* dll_handle){
    #ifdef WINDOWS
    FreeLibrary(dll_handle);
    #else
    dlclose(dll_handle);
    #endif
}

Object import_dll(Executor* E, const char* library_name){
    typedef Object (*ModuleInitFunction) (Executor*);
    Object result;
    #ifdef WINDOWS
    HINSTANCE lib_handle = LoadLibrary(library_name);

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
        Object err;
        NEW_ERROR(err, "DLL_IMPORT_ERROR", to_string(library_name), (char*)lpMsgBuf)
        return err;
    }
    #else
    void* lib_handle = dlopen(library_name, RTLD_LAZY);
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
        result=init_function(E);
    } else {
        result=null_const;
    }
    //dlclose(lib_handle);
    #endif
    return result;
}