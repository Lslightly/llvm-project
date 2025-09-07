#include "ctx.h"
#include <stdlib.h>

__attribute__((constructor()))
void stub_init() {
    alloc_enter(0);
    alloc_exit();
    void* ptr = malloc_wrapper(0, 0);
    free(ptr);
}
