#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "mv_assert.h"
#include "platform.h"

static uintptr_t pagesize;

void *multiverse_os_addr_to_page(void *addr) {
}

/**
   @brief Enable the memory protection of a page
*/
void multiverse_os_protect(void * page) {
}

/**
   @brief Disable the memory protection of a page
*/
void multiverse_os_unprotect(void * page) {
}


void multiverse_os_clear_cache(void* addr, unsigned int length) {
}


void* multiverse_os_malloc(size_t size) {
    return 0;
}


void* multiverse_os_calloc(size_t num, size_t size) {
    return 0;
}


void multiverse_os_free(void* ptr) {
    free(ptr);
}


void* multiverse_os_realloc(void* ptr, size_t size) {
    return 0;
}


void multiverse_os_print(const char* fmt, ...) {
    /* va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);*/
}
