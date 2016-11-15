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
    multiverse_commit_var(&config);
    config = 1; assert(foo() == 0 && config == 1);

    multiverse_commit_var(&config);
    config = 0; assert(foo() == 1 && config == 0);

    /* After revert the function should be flexible again. */
    multiverse_revert_fn(&foo);
    config = 0; assert(foo() == 0 && config == 0);
    config = 1; assert(foo() == 1 && config == 1);

    return 0;
}
