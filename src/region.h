#ifndef REGION_H_
#define REGION_H_

#include <stdlib.h>
#include "sv.h"

#define REGION_CAPACITY (1 * 1000 * 1000)

typedef struct {
    size_t size;
    char memory[REGION_CAPACITY];
} Region;

void *region_malloc(Region *region, size_t size);
void *region_realloc(Region *region, void *old_memory, size_t old_size, size_t new_size);
void region_clean(Region *region);
char *region_cstr_from_sv(Region *region, String_View sv);
char *region_slurp_file(Region *region, const char *file_path);

#endif // REGION_H_
