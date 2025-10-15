#ifndef DEMO_ALLOC_H
#define DEMO_ALLOC_H

#include <bitset>

const int PAGE_SIZE = 4096;
const int SIZE_CLASS = 96;

typedef struct GrpState {
    std::bitset<PAGE_SIZE/SIZE_CLASS+1> bitmap; // 1: allocated, 0: freed
    unsigned char* slab_ptr;
    unsigned char* slab_end;
    bool contains(void* ptr);
} GrpState;

thread_local GrpState GlobalGrpState = GrpState{
    .bitmap = {},
    .slab_ptr = nullptr,
    .slab_end = nullptr,
};

void initGrp(GrpState* grp_state);

void m_init();


#endif