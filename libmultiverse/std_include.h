#define KERNEL_SPACE 1
#define PAGE_SIZE 4096

#ifdef KERNEL_SPACE

#include <stdint.h>
#include <stddef.h>

int memcmp(const void*, const void*, size_t);

void* memcpy(void *, const void *, size_t);

void* memmove(void *, const void *, size_t);


void* malloc(size_t size);

void* calloc(size_t num, size_t size);

void free(void* ptr);

void* realloc(void* ptr, size_t size);

unsigned long kallsyms_lookup_name(const char *name);

// #include <stdarg.h>
// int vprintk(const char *fmt, va_list args);
//
// static void print(const char* fmt, ...) {
//   va_list args;
//   va_start(args, fmt);
//   vprintk(fmt, args);
//   va_end(args);
// }

#else

#include <memory.h>
#include <stdint.h>
#include <memory.h>

#endif
