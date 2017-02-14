/*
 * The compiler throws a warning in case a multiverse function does not
 * reference any multiverse variable.
 */

#include <stdio.h>
#include "multiverse.h"
#include "testsuite.h"

typedef enum { false, true } bool;


void __attribute__((multiverse)) foo()
{
    for (int i=0; i < 100; i++) {
        // Do nothing
        printf("i=%d\n", i);
    }
}


int main(int argc, char **argv)
{
    multiverse_init();

    foo();

    return 0;
}
