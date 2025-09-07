#include "ctx.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

THREAD_LOCAL size_t TLS_GRP = 0;

#ifdef __cplusplus
extern "C" {
#endif

void* malloc_wrapper(size_t size, size_t grp) {
    alloc_enter(grp);
    void* ptr = malloc(size);
    alloc_exit();
    return ptr;
}

void* new_wrapper(size_t size, size_t grp) {
    return malloc_wrapper(size, grp);
}

#ifdef __cplusplus
}
#endif
