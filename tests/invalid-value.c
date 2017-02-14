/*
 * By default, unsigned numerical types (e.g., char, int) are treated like
 * booleans and referencing functions are thus specialized for the variable
 * settings [0,1].
 *
 * In case a variable's value exceeds the [0,1] range while committing, the
 * run-time system falls back to the generic function.
 */

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

    // Check the static property of multiverse variants
    config = 1;
    multiverse_commit_refs(&config);
    config = 23; assert(foo() == 1 && config == 23);

    multiverse_commit_refs(&config);    // 23 > [0,1] : fall back to generic foo()
    assert(foo() == 23 && config == 23);
    config = 42; assert(foo() == 42 && config == 42);

    // Reverting won't change foo, since it's already using the generic version
    int x = multiverse_revert_fn(&foo);
    assert(x == 0);

    return 0;
}
