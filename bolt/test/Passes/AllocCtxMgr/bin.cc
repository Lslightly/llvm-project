#include <cstdlib>

#define NOINLINE __attribute__((noinline))

extern "C" NOINLINE
void foo() {
    int* p = (int*)malloc(sizeof(int));
    free(p);
}

int main() {
    foo();
    return 0;
}
