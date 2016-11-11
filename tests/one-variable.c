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


    multiverse_dump_info(stderr);


    struct mv_info_var *var_info = multiverse_info_var(&config);
    assert(var_info != NULL);

    struct mv_info_fn *foo_info = multiverse_info_fn(&foo);
    assert(foo_info != NULL);
    assert(foo_info->n_mv_functions == 2);

    /* Check the static property of multiverse variants */
    assert(foo() == 0 && config == 0);

    multiverse_commit_fn(foo_info);

    assert(foo() == 0 && config == 0);

    config=1;

    assert(foo() == 0 && config == 1);

    multiverse_commit();

    assert(foo() == 1 && config == 1);

    return 0;
}
