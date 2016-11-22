/*
 * Show how committing & reverting a multiverse variable works.
 *
 * The expected output is:
 *      configA=1  | generic
 *      configA=0  | committed and optimized
 *      configA=0  | still optimized although configA=1
 *      configA=1  | reverted to generic
 */

#include <stdio.h>
#include "multiverse.h"
#include "testsuite.h"

typedef enum { false, true } bool;

bool __attribute__((multiverse)) configA;


void __attribute((multiverse)) foo()
{
    printf("foo(): ");
    if (configA)
        printf("configA=1\n");
    else
        printf("configA=0\n");
}


int main(int argc, char **argv)
{
    multiverse_init();

    configA = 1;
    foo();  // generic function

    configA = 0;
    multiverse_commit_refs(&configA);
    foo();  // optimized version for configA=0

    configA = 1;
    foo();  // optimized version for configA=0, although configA is now 1

    multiverse_revert_refs(&configA);
    foo();  // generic function

    return 0;
}
