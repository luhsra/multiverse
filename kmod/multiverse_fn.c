#include <stdbool.h>
#include "../libmultiverse/multiverse.h"
#include <stdarg.h>

int vprintk(const char *fmt, va_list args);

static void print(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintk(fmt, args);
  va_end(args);
}


static __attribute__((multiverse)) bool config_A = false;
static __attribute__((multiverse)) bool config_B = false;

void __attribute__((multiverse)) foo(void) {
    label:
    print("foo at: %p\n", &&label);
    if (config_A)
        print(">>> config_A\n");
    if (config_B)
        print(">>> config_B\n");
}


void multiverse_fn(void) {
    multiverse_init();

    config_A = true;
    multiverse_commit_refs(&config_A);
    foo();
    config_B = true;
    multiverse_commit_refs(&config_B);
    foo();
}
