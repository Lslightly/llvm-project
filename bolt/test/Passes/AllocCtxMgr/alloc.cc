#include <cstddef>
#include <new>
#include "dlfcn.h"


#include "ctx.h"

static void *(*libc_malloc)(size_t);
static void (*libc_free)(void *);

thread_local bool InMalloc = false;

inline bool useGrpAlloc(size_t grp) {
    return grp != TLS_NOGRP;
}

#ifdef __cplusplus
extern "C" {
#endif

void* malloc(size_t size) {
    size_t grp = TLS_GRP;
    if (InMalloc || !useGrpAlloc(grp)) {
        return libc_malloc(size);
    };
    InMalloc = true;
    InMalloc = false;
}

void free(void* ptr) {
    
}

#ifdef __cplusplus
}
#endif



void* operator new(size_t size) noexcept(false) {
    return malloc(size);
}

void operator delete(void* ptr) {
    free(ptr);
}

void __attribute__((constructor)) m_init(void) {
    libc_malloc = (void * ( *)(size_t))dlsym(RTLD_NEXT, "malloc");
    libc_free = (void ( *)(void *))dlsym(RTLD_NEXT, "free");
}