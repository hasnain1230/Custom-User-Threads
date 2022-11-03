#include "checkMalloc.h"
#include <stdio.h>
#include <stdlib.h>

void checkMalloc(void *ptr) { // Checks to see if malloc was successful. Fails if malloc failed. This is a helper function.
    if (ptr == NULL) {
        perror("Malloc failed.");
        exit(1);
    }
}
