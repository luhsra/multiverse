/*
 * The run-time library offers the possibility to revert functions to their
 * generic versions via:
 *  - multiverse_revert_fn(&func) : revert function 'func'
 *  - multiverse_revert_refs(&var): revert all functions referencing 'var'
 *  - multiverse_revert(void)     : revert all functions unconditionally
 *
 * This test demonstrates the semantics of those three revert functions.
 */

#include <stdio.h>
#include "multiverse.h"
#include "testsuite.h"

typedef enum {true, false} bool;

__attribute__((multiverse)) bool conf_a;
__attribute__((multiverse)) bool conf_b;


int __attribute((multiverse)) func_a()
{
    return conf_a;
}


int __attribute((multiverse)) func_b()
{
    return conf_b;
}


int __attribute((multiverse)) func_c()
{
    return conf_b || conf_a;
}


int main(int argc, char **argv)
{
    multiverse_init();

    // Check the static property of multiverse variants
    multiverse_commit_refs(&conf_a);
    assert(func_a() == 0 && conf_a == 0);

    multiverse_commit_refs(&conf_b);
    assert(func_b() == 0 && conf_b == 0);

    // Use *_revert_fn() to revert a specific functions
    multiverse_revert_fn(&func_a);
    assert(func_a() == 0 && conf_a == 0);
    conf_a = 1; assert(func_a() == 1);

    multiverse_commit_refs(&conf_a);
    conf_a = 0; assert(func_a() == 1);

    // Use *_revert_refs() to revert referencing functions
    assert(func_c() == 1);  // conf_a=1, conf_b=0
    conf_a = 1; conf_b = 1; assert(func_c() == 1);  // it's still committed
    multiverse_revert_refs(&conf_b);  // func_b & func_c will be reverted
    assert(func_b() == 1 && func_c() == 1);

    // Use *_revert() to revert all functions
    conf_a = 0; conf_b = 1;
    multiverse_commit();  // first commit everything
    assert(func_a() == 0 && func_b() == 1 && func_c() == 1);

    // now revert all functions and test each of them
    multiverse_revert();
    conf_a = 0; assert(func_a() == 0);
    conf_a = 1; assert(func_a() == 1);

    conf_b = 0; assert(func_b() == 0);
    conf_b = 1; assert(func_b() == 1);

    conf_a = 0; conf_b = 0; assert(func_c() == 0);
    conf_a = 1; conf_b = 0; assert(func_c() == 1);
    conf_a = 0; conf_b = 1; assert(func_c() == 1);
    conf_a = 1; conf_b = 1; assert(func_c() == 1);

    return 0;
}
