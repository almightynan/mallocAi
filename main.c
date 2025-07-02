#include "mallocai/mallocai.h"
#include <stdio.h>

int main() {
    void *p = mallocAi("allocate memory for test");
    if (!p) {
        printf("Allocation failed\n");
        return 1;
    }
    printf("Allocation succeeded\n");
    free(p);
    return 0;
}
