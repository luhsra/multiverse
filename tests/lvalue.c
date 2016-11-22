/*
 * If a multiverse variable is the lvalue of an assignment, it must be removed
 * from the variable setting of the function.
 */


#include <assert.h>
#include <stdio.h>

#include "multiverse.h"

typedef enum { false, true } bool;

bool __attribute__((multiverse)) configA;


void __attribute__((multiverse)) lvalue_func()
{
    int foo = 1;

    if (configA)
        foo = configA;

    configA = false;
}


int main(int argc, char **argv)
{
    multiverse_init();

    multiverse_dump_info(stderr);

    lvalue_func();

    return 0;
}
