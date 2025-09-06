#include "ctx.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void alloc_enter(size_t grp) {
    printf("alloc enter %ld\n", grp);
}

void alloc_exit(size_t grp) {
    printf("alloc exit %ld\n", grp);
}

void* malloc_wrapper(size_t size, size_t grp) {
    alloc_enter(grp);
    void* ptr = malloc(size);
    alloc_exit(grp);
    return ptr;
}

#ifdef __cplusplus
}
#endif
