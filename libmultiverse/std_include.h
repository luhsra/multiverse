#define KERNEL_SPACE 1
#define PAGE_SIZE 4096

#ifdef KERNEL_SPACE

#include <stdint.h>
#include <stddef.h>

extern int memcmp(const void*, const void*, size_t);

extern void* memcpy(void *, const void *, size_t);

extern void* memmove(void *, const void *, size_t);


extern void* malloc(size_t size);

extern void* calloc(size_t num, size_t size);

extern void free(void* ptr);

extern void* realloc(void* ptr, size_t size);

#else

#include <memory.h>
#include <stdint.h>
#include <memory.h>

#endif
