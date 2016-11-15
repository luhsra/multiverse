#include <stdio.h>
#include "multiverse.h"
#include "testsuite.h"

__attribute__((multiverse)) unsigned char config;

int copy_config;

int __attribute((multiverse, noinline)) foo()
{
    /* if (config) {
       printf("config = true, %d\n", config);
       } else {
       printf("config = false, %d\n", config);
       } */

    return config;
}


int main(int argc, char **argv)
{
    multiverse_init();

    /* Check the static property of multiverse variants */
    assert(foo() == 0 && config == 0);

    config = 1;
    int x = multiverse_commit_var(&config);
    assert(x == 1);

    assert(foo() == 1 && config == 1);

    x = multiverse_commit_var(&config);
    assert(x == 0);

    return 0;
}
