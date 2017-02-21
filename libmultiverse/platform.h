/**
   @file The operating system specific stubs.

   The operating system specific parts include the handling of memory
   protection and cache handling.
*/
#ifndef __MULTIVERSE_PLATFORM_H
#define __MULTIVERSE_PLATFORM_H

#include "mv_commit.h"

/**
 @brief Translate a pointer to a page pointer of the desired OS configuration
*/
void *multiverse_os_addr_to_page(void *);

/**
 @brief Enable the memory protection of a page
*/
void multiverse_os_protect(void * page);

/**
   @brief Disable the memory protection of a page
*/
void multiverse_os_unprotect(void * page);

/**
 @brief Clear all instruction cache lines that cover [addr, addr+length]
*/
void multiverse_os_clear_cache(void* addr, unsigned int length);

#endif
