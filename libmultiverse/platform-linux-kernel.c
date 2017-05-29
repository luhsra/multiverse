/* #include <assert.h> */
#include <stdarg.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/slab.h>
#include "mv_assert.h"
#include "platform.h"


void *multiverse_os_addr_to_page(void *addr) {
    void *page = (void*)((uintptr_t)addr & ~(PAGE_SIZE - 1));
    return page;
}

/**
   @brief Enable the memory protection of a page
*/
void multiverse_os_protect(void * page) {
    static int (*set_memory_ro)(unsigned long addr, int numpages) = (void*)0;
    int ret;
    if (set_memory_ro == (void*)0) {
        set_memory_ro = (void*)kallsyms_lookup_name("set_memory_ro");
        MV_ASSERT(set_memory_ro != (void*)0);
    }
    ret = set_memory_ro((unsigned long)page, 1);
    MV_ASSERT(ret == 0);
}

/**
   @brief Disable the memory protection of a page
*/
void multiverse_os_unprotect(void * page) {
    static int (*set_memory_rw)(unsigned long addr, int numpages) = (void*)0;
    int ret;
    if (set_memory_rw == (void*)0) {
        set_memory_rw = (void*)kallsyms_lookup_name("set_memory_rw");
        MV_ASSERT(set_memory_rw != (void*)0);
    }
    ret = set_memory_rw((unsigned long)page, 1);
    MV_ASSERT(ret == 0);
}


void multiverse_os_clear_cache(void* addr, unsigned int length) {
    __builtin___clear_cache(addr, addr+length);
}


void* multiverse_os_malloc(size_t size) {
    return kmalloc(size, GFP_KERNEL);
}


void* multiverse_os_calloc(size_t num, size_t size) {
    return kcalloc(num, size, GFP_KERNEL);
}


void multiverse_os_free(void* ptr) {
    kfree(ptr);
}


void* multiverse_os_realloc(void* ptr, size_t size) {
    return krealloc(ptr, size, GFP_KERNEL);
}


void multiverse_os_print(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);
}
