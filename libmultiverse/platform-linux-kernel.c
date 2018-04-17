/* #include <assert.h> */
#include <stdarg.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/slab.h>
#include <linux/bootmem.h>
#include "multiverse.h"
#include "mv_assert.h"
#include "platform.h"

EXPORT_SYMBOL(multiverse_init);
EXPORT_SYMBOL(multiverse_dump_info);
EXPORT_SYMBOL(multiverse_commit_info_fn);
EXPORT_SYMBOL(multiverse_commit_fn);
EXPORT_SYMBOL(multiverse_commit_info_refs);
EXPORT_SYMBOL(multiverse_commit_refs);
EXPORT_SYMBOL(multiverse_commit);
EXPORT_SYMBOL(multiverse_revert_info_fn);
EXPORT_SYMBOL(multiverse_revert_fn);
EXPORT_SYMBOL(multiverse_revert_info_refs);
EXPORT_SYMBOL(multiverse_revert_refs);
EXPORT_SYMBOL(multiverse_revert);
EXPORT_SYMBOL(multiverse_is_committed);
EXPORT_SYMBOL(multiverse_bind);


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

void multiverse_os_clear_caches(void) { }


void* multiverse_os_malloc(size_t size) {
    // The slab allocator is not available at boot time.  We use the early
    // bootmem allocator in this case.  Note that memory from alloc_bootmem
    // cannot be freed later.  This is currently no problem because Multiverse
    // only needs dynamic memory for initialising persistent data.
    if (slab_is_available()) {
        return kmalloc(size, GFP_KERNEL);
    } else {
        return alloc_bootmem(size);
    }
}


void multiverse_os_print(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);
}
