/*
 * A very simple test case with
 *  - one boolean variable 'config'
 *  - one function 'foo' that returns 'config'
 *
 * The test also checks some multiverse run-time data.
 */

#include <stdio.h>
#include "multiverse.h"
#include "testsuite.h"

typedef _Bool bool;

__attribute__((multiverse)) bool config;


int __attribute((multiverse, noinline)) foo()
{
    return config;
}


int main(int argc, char **argv)
{
    multiverse_init();

    // Get the mv_info_var struct for config
    struct mv_info_var *var_info = multiverse_info_var(&config);
    assert(var_info != NULL);

    /*
     * Get the mv_info_fn struct for foo().  Since the mv_variable config is an
     * "untracked" bool, there must be two mv_functions referencing this
     * variable (config=0 and config=1).
     */
    struct mv_info_fn *foo_info = multiverse_info_fn(&foo);
    assert(foo_info != NULL);
    assert(foo_info->n_mv_functions == 2);

    // Check the static property of multiverse variants
    assert(foo() == 0 && config == 0);

    multiverse_commit_info_fn(foo_info);

    assert(foo() == 0 && config == 0);

    /*
     * Change config's value. foo() must still return the old value (0) since we
     * did not commit the new value.
     */
    config = 1;

    assert(foo() == 0 && config == 1);

    // Commit config's new value
    multiverse_commit_refs(&config);

    assert(foo() == 1 && config == 1);

    return 0;
}
