#include "prelude.h"

#include <stdlib.h>
#include <stdio.h>

void *alloc_mem(usize len)
{
    void *ptr = malloc(len);
    if (ptr == null) {
        exit(EXIT_FAILURE);
    }
    return ptr;
}
void *realloc_mem(void* ptr, usize len)
{
    void *new_ptr = realloc(ptr, len);
    if (new_ptr == null && len > 0) {
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}

void free_mem(void* ptr)
{
    free(ptr);
}

void panic(const char* msg)
{
    printf("%s\n", msg);
    exit(EXIT_FAILURE);
}
