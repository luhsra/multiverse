#include <stdio.h>
#include "multiverse.h"
#include "testsuite.h"

__attribute__((multiverse)) unsigned char config;

int __attribute((multiverse, noinline)) foo()
{
    return config;
}

int main(int argc, char **argv)
{
    multiverse_init();

    /* Check the static property of multiverse variants */
    multiverse_commit_refs(&config);
    config = 23; assert(foo() == 0 && config == 23);

    multiverse_commit_refs(&config);
    assert(foo() == 23 && config == 23);
    config = 42; assert(foo() == 42 && config == 42);

    /* After revert the function should be flexible again. */
    int x = multiverse_revert_fn(&foo);
    assert(x == 0);

    return 0;
}
