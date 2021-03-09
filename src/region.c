#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "./region.h"

void *region_malloc(Region *region, size_t size)
{
    if (region->size + size >= REGION_CAPACITY) {
        errno = ENOMEM;
        return NULL;
    }

    void *result = region->memory + region->size;
    region->size += size;
    return result;
}

void *region_realloc(Region *region, void *old_memory, size_t old_size, size_t new_size)
{
    void *new_memory = region_malloc(region, new_size);

    if (old_size > new_size) {
        old_size = new_size;
    }

    memcpy(new_memory, old_memory, old_size);
    return new_memory;
}

void region_clean(Region *region)
{
    region->size = 0;
}

char *region_cstr_from_sv(Region *region, String_View sv)
{
    char *result = region_malloc(region, sv.count + 1);
    memcpy(result, sv.data, sv.count);
    result[sv.count] = '\0';
    return result;
}

char *region_slurp_file(Region *region, const char *file_path)
{
    FILE *f = NULL;
    char *buffer = NULL;

    f = fopen(file_path, "r");
    if (f == NULL) goto end;
    if (fseek(f, 0, SEEK_END) < 0) goto end;

    long size = ftell(f);
    if (size < 0) goto end;

    buffer = region_malloc(region, size + 1);
    if (buffer == NULL) goto end;

    if (fseek(f, 0, SEEK_SET) < 0) goto end;

    fread(buffer, 1, size, f);
    if (ferror(f) < 0) goto end;

    buffer[size] = '\0';

end:
    if (f) fclose(f);
    return buffer;
}
