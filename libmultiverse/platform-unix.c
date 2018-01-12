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
    if (pagesize == 0) {
        pagesize = sysconf(_SC_PAGESIZE);
    }
    void *page = (void*)((uintptr_t) addr & ~(pagesize - 1));
    return page;
}

/**
   @brief Enable the memory protection of a page
*/
void multiverse_os_protect(void * page) {
    MV_ASSERT(pagesize != 0);
    if (mprotect(page, pagesize, PROT_READ | PROT_EXEC)) {
        MV_ASSERT(0 && "mprotect should not fail");
    }
}

/**
   @brief Disable the memory protection of a page
*/
void multiverse_os_unprotect(void * page) {
    MV_ASSERT(pagesize != 0);
    if (mprotect(page, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC)) {
        MV_ASSERT(0 && "mprotect should not fail");
    }
}


void multiverse_os_clear_cache(void* addr, unsigned int length) {
    __builtin___clear_cache(addr, addr+length);
}

void multiverse_os_clear_caches() { }


void* multiverse_os_malloc(size_t size) {
    return malloc(size);
}


void* multiverse_os_calloc(size_t num, size_t size) {
    return calloc(num, size);
}


void multiverse_os_free(void* ptr) {
    free(ptr);
}


void* multiverse_os_realloc(void* ptr, size_t size) {
    return realloc(ptr, size);
}


void multiverse_os_print(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}
