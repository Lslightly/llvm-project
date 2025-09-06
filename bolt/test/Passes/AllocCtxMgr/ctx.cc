#include "ctx.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

THREAD_LOCAL size_t TLS_GRP = 0;

#ifdef __cplusplus
extern "C" {
#endif

/*
TODO:
    1. bolt rewrite pass
    2. make_shared primitive test case
*/
void alloc_enter(size_t grp) {
    printf("alloc enter %ld\n", grp);
    TLS_GRP = grp;
}

void alloc_exit(size_t grp) {
    printf("alloc exit %ld\n", grp);
    TLS_GRP = TLS_NOGRP;
}

void* malloc_wrapper(size_t size, size_t grp) {
    alloc_enter(grp);
    void* ptr = malloc(size);
    alloc_exit(grp);
    return ptr;
}

void* new_wrapper(size_t size, size_t grp) {
    return malloc_wrapper(size, grp);
}

#ifdef __cplusplus
}
#endif
