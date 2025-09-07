#include <cstddef>
#include <cstdio>
#include <new>
#define _GNU_SOURCE
#include "dlfcn.h"


#include "ctx.h"
#include "alloc.hh"

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
    if (!libc_malloc) {
        m_init();
    }
    size_t grp = TLS_GRP;
    if (InMalloc || !useGrpAlloc(grp)) {
        return libc_malloc(size);
    };
    InMalloc = true;
    printf("grp %lu\n", grp);
    void* ptr = libc_malloc(size);
    InMalloc = false;
    return ptr;
}

void free(void* ptr) {
    libc_free(ptr);
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