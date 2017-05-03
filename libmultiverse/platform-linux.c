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
    /* if (mprotect(page, pagesize, PROT_READ | PROT_EXEC)) { */
    /*     assert(0 && "mprotect should not fail"); */
    /* } */
}

/**
   @brief Disable the memory protection of a page
*/
void multiverse_os_unprotect(void * page) {
    /* if (mprotect(page, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC)) { */
    /*     assert(0 && "mprotect should not fail"); */
    /* } */
}

void multiverse_os_clear_cache(void* addr, unsigned int length) {
    __builtin___clear_cache(addr, addr+length);
}
