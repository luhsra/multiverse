/*
 * Use multiverse_commit_var(&variable) to commit that a multiverse variable is
 * stable.  This function returns the number of changed functions and -1 in case
 * of error.
 *
 * TODO: add multiverse-tracked semantic
 */

#include <stdio.h>
#include "multiverse.h"
#include "testsuite.h"

typedef enum { a=0, b, c, d, e } state;


__attribute__((multiverse)) state configA;
__attribute__((multiverse)) state configB;


int __attribute((multiverse)) foo()
{
    printf("configA=%d, configB=%d\n", configA, configB);
    return configA + configB;
}


int main(int argc, char **argv)
{
    multiverse_init();

    configA = a; // 0
    multiverse_commit_refs(&configA);  // TODO: add multiverse-tracked semantic
    foo();      // configA=0, configB=0

    configB = b; // 1
    foo();      // configA=0, configB=1
    multiverse_commit_refs(&configB);
    foo();      // configA=0, configB=1

    return 0;
}
