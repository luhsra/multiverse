#include "multiverse.h"

typedef enum {true, false} bool;

__attribute__((multiverse)) bool conf_a;
__attribute__((multiverse)) bool conf_b;

int __attribute((multiverse)) func_a();
int __attribute((multiverse)) func_b();
