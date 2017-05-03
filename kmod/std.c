#include <linux/types.h>
#include <linux/string.h>
#include <linux/slab.h>

void* malloc(size_t size) {
    return kmalloc(size, GFP_KERNEL);
}

void* calloc(size_t num, size_t size) {
    return kcalloc(num, size, GFP_KERNEL);
}

void free(void* ptr) {
    kfree(ptr);
}

void* realloc(void* ptr, size_t size) {
    return krealloc(ptr, size, GFP_KERNEL);
}
