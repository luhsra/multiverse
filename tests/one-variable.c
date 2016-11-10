#include <stdio.h>
#include "multiverse.h"
#include "testsuite.h"

__attribute__((multiverse)) unsigned char config;

int __attribute((multiverse)) foo()
{
    if (config) {
        printf("config = true\n");
    } else {
        printf("config = false\n");
    }

    return 42;
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

    foo();

    multiverse_commit_fn(foo_info);

    config = 1;
    foo();

    multiverse_commit();

    foo();

    return 0;
}
