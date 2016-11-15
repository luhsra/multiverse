#include <stdio.h>
#include "multiverse.h"
#include "testsuite.h"

__attribute__((multiverse)) unsigned char config;

int __attribute((multiverse, noinline)) foo()
{
    return config;
}

int (*foo_ptr)() = foo;

int main(int argc, char **argv)
{
    multiverse_init();

    /* Check the static property of multiverse variants */
    multiverse_commit_var(&config);
    config = 1; assert(foo_ptr() == 0 && config == 1);

    multiverse_commit_fn(&foo);
    config = 0; assert(foo_ptr() == 1 && config == 0);

    return 0;
}
