#include <stdlib.h>
#include <stdio.h>
#include "mallocai/mallocai.h"

int main() {
    void *ptr = mallocAi_verbose("enough for an array of 4 int", 1);
    if (ptr) {
        printf("Allocated: %p\n", ptr);
        free(ptr);
    }
    return 0;
}
