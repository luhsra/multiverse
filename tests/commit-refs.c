/*
 * Use multiverse_commit_refs(&variable) to commit that a multiverse variable is
 * stable.  This function only patches functions that reference var, and returns
 * the number of changed functions and -1 in case of error.
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


int main(int argc, char **argv)
{
    int num_changed_funcs = 0;

    multiverse_init();
    assert(func_a() == 0 && conf_a == 0);
    assert(func_b() == 0 && conf_b == 0);

    // Call multiverse_commit_refs on conf_a: only func_a will be patched
    conf_a = 1;
    num_changed_funcs = multiverse_commit_refs(&conf_a);
    assert(num_changed_funcs == 1); // only one function (func_a) has been patched
    assert(func_a() == 1);

    conf_a = 0;
    assert(func_a() == 1);  // func_a has already been specialized with conf_a=1

    assert(func_b() == 0);
    conf_b = 1;
    assert(func_b() == 1);  // func_b hasn't been patched and still uses the generic version

    num_changed_funcs = multiverse_commit_refs(&conf_b);
    assert(num_changed_funcs == 1); // only one function (func_a) has been patched
    conf_b = 0;
    assert(func_b() == 1);  // func_b has already been specialized with conf_b=1

    // Test return value in case of error
    bool dummy = 0;
    int error = 0;
    error = multiverse_commit_refs(&dummy);
    assert(error == -1);

    return 0;
}
