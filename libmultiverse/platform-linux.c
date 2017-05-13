#include <stdint.h>
/* #include <assert.h> */
#include "std_include.h"
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
    if (set_memory_ro == (void*)0) {
        set_memory_ro = (void*)kallsyms_lookup_name("set_memory_ro");
    }
    // TODO this may fail
    set_memory_ro((unsigned long)page, 1);
}

/**
   @brief Disable the memory protection of a page
*/
void multiverse_os_unprotect(void * page) {
    static int (*set_memory_rw)(unsigned long addr, int numpages) = (void*)0;
    if (set_memory_rw == (void*)0) {
        set_memory_rw = (void*)kallsyms_lookup_name("set_memory_rw");
    }
    // TODO this may fail
    set_memory_rw((unsigned long)page, 1);
}

void multiverse_os_clear_cache(void* addr, unsigned int length) {
    __builtin___clear_cache(addr, addr+length);
}
