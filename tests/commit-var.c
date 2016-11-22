/*
 * Use multiverse_commit_var(&variable) to commit that a multiverse variable is
 * stable.  This function returns the number of changed functions and -1 in case
 * of error.
 */

#include <stdio.h>
#include "multiverse.h"
#include "testsuite.h"

__attribute__((multiverse)) unsigned char config;


int __attribute((multiverse)) foo()
{
    return config;
}


int main(int argc, char **argv)
{
    int num_changed_funcs = 0;

    multiverse_init();

    /* Check the static property of multiverse variants */
    assert(foo() == 0 && config == 0);

    config = 1;
    num_changed_funcs = multiverse_commit_var(&config);
    assert(num_changed_funcs == 1);

    assert(foo() == 1 && config == 1);

    num_changed_funcs = multiverse_commit_var(&config);
    assert(num_changed_funcs == 0);

    return 0;
}
