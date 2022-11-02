#include "checkMalloc.h"
#include <stdio.h>
#include <stdlib.h>

void checkMalloc(void *ptr) {
    if (ptr == NULL) {
        perror("Malloc failed.");
        exit(1);
    }
}
