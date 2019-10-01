#ifndef WINDOWS_POLYFILLS
#define WINDOWS_POLYFILLS

#include <windows.h>
#include <errno.h>
#include <malloc.h>

void * _aligned_malloc(
    size_t size,
    size_t alignment
);

#define posix_memalign(p, a, s) (((*(p)) = _aligned_malloc((s), (a))), *(p) ?0 :errno)

#define _SC_PAGE_SIZE 0
long sysconf(int name) {
    if(name==_SC_PAGE_SIZE){
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        return info.dwPageSize;
    } else {
        errno=EINVAL;
        return -1;
    }
}

#endif