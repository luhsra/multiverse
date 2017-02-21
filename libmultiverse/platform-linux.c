#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
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
    assert(pagesize != 0);
    if (mprotect(page, pagesize, PROT_READ | PROT_EXEC)) {
        assert(0 && "mprotect should not fail");
    }
}

/**
   @brief Disable the memory protection of a page
*/
void multiverse_os_unprotect(void * page) {
    assert(pagesize != 0);
    if (mprotect(page, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC)) {
        assert(0 && "mprotect should not fail");
    }
}

void multiverse_os_clear_cache(void* addr, unsigned int length) {
    __builtin___clear_cache(addr, addr+length);
}
