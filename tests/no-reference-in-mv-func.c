/*
 * If a multiverse variable is the lvalue of an assignment, it must be removed
 * from the variable setting of the function.
 */


#include <assert.h>
#include <stdio.h>

#include "multiverse.h"

typedef enum { false, true } bool;

void __attribute__((multiverse)) foo()
{
    for (int i=0; i < 100; i++) {
        // Do nothing
        i++;
    }
}


int main(int argc, char **argv)
{
    multiverse_init();

    multiverse_dump_info(stderr);

    foo();

    return 0;
}
