#include <stdarg.h>
#include "lib/kmalloc.h"
#include "lib/printk.h"
#include "lib/memset.h"
#include "hw/hal/PageMap.h"
#include "hw/hal/MapFlags.h"



#include "mv_assert.h"
#include "platform.h"

void *multiverse_os_addr_to_page(void *addr) {
    void *page = (void*)((uintptr_t) addr & ~(PAGE_SIZE - 1));
    return page;
}

/**
   @brief Enable the memory protection of a page
*/
void multiverse_os_protect(void * page) {
    auto pagemap = hw::hal::Pagemap::getCurrent();
    auto flags = hw::hal::MapFlags();
    flags.writeable(false);
    flags.present(true);
    pagemap->mapFlags((uintptr_t)page, flags);
    pagemap->invalidate((uintptr_t)page);
}

/**
   @brief Disable the memory protection of a page
*/
void multiverse_os_unprotect(void * page) {
    auto pagemap = hw::hal::Pagemap::getCurrent();
    auto flags = hw::hal::MapFlags();
    flags.writeable(true);
    flags.present(true);
    pagemap->mapFlags((uintptr_t)page, flags);
    pagemap->invalidate((uintptr_t)page);
}


void multiverse_os_clear_cache(void* addr, unsigned int length) {
    __builtin___clear_cache(addr, (void*)((uintptr_t)addr+length));
}

void multiverse_os_clear_caches() {
    asm ("wbinvd");
}


void* multiverse_os_malloc(size_t size) {
    return kmalloc_raw(size);
}


void* multiverse_os_calloc(size_t num, size_t size) {
    void *ret = kmalloc_raw(size);
    if (ret)
        lib::memset(ret, 0, size);
    return ret;
}


void multiverse_os_print(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);
}
