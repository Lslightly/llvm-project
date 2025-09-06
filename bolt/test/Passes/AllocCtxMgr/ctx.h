#ifndef ALLOC_CTX_H
#define ALLOC_CTX_H
#include <stddef.h>

#define THREAD_LOCAL __thread

extern THREAD_LOCAL size_t TLS_GRP;
const size_t TLS_NOGRP = 0; // No Group signal

#ifdef __cplusplus
extern "C" {
#endif

void alloc_enter(size_t grp);
void alloc_exit(size_t grp);
void* malloc_wrapper(size_t size, size_t grp);
void* new_wrapper(size_t size, size_t grp);

#ifdef __cplusplus
}
#endif

#endif