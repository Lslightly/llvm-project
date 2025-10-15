#include <cassert>
#include <cstddef>
#include <cstdio>
#include <new>
#include <sys/mman.h>
#define _GNU_SOURCE
#include "dlfcn.h"


#include "ctx.h"
#include "demo_alloc.hh"

static void *(*libc_malloc)(size_t);
static void (*libc_free)(void *);

thread_local bool InMalloc = false;

inline bool useGrpAlloc(size_t grp) {
    return grp != TLS_NOGRP;
}

void initGrp(GrpState *grp_state) {
    for (size_t i = 0; i < grp_state->bitmap.size(); i++) {
        grp_state->bitmap.reset();
    }
    auto* base = (unsigned char*)mmap(nullptr, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
    assert(base != MAP_FAILED);

    grp_state->slab_ptr = base;
    grp_state->slab_end = base + PAGE_SIZE;
    return;
}

bool GrpState::contains(void* ptr) {
    return this->slab_ptr <= ptr && ptr < this->slab_end;
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
    if (size != SIZE_CLASS) {
        return libc_malloc(size);
    }
    if (GlobalGrpState.slab_ptr == nullptr) {
        initGrp(&GlobalGrpState);
    }
    InMalloc = true;
    auto& bitmap = GlobalGrpState.bitmap;
    void* ptr = nullptr;
    bool has_free = false;
    for (size_t i = 0; i < bitmap.size(); i++) {
        if (!bitmap.test(i)) {
            ptr = (void*)(GlobalGrpState.slab_ptr + SIZE_CLASS*i);
            bitmap.set(i);
            has_free = true;
            break;
        }
    }
    if (!has_free) {
        ptr = libc_malloc(size);
    }
    InMalloc = false;
    return ptr;
}

void free(void* ptr) {
    if (!GlobalGrpState.contains(ptr)) {
        libc_free(ptr);
        return;
    }
    auto idx = ((unsigned char*)ptr - GlobalGrpState.slab_ptr) / SIZE_CLASS;
    GlobalGrpState.bitmap.set(idx);
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