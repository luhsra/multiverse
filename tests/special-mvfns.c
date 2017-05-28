/*
 * If a multiverse function body returns only a constant, or is not
 * present at all, the callsite can be replaced (on some
 * architectures) by a plain move operation or nop.
 */

#include <assert.h>
#include <stdio.h>
#include "multiverse.h"

typedef enum { false, true } bool;

bool __attribute__((multiverse)) configA;


void __attribute__((multiverse)) empty_variant()
{
    if (configA) {
        printf("Print Something!");
    }
}

int __attribute__((multiverse)) constant()
{
    if (configA) {
        return 42;
    }
    return 0;
}


int main(int argc, char **argv)
{
    multiverse_init();

    multiverse_dump_info();
    multiverse_commit();

    empty_variant();
    if (constant()) { printf("Just to have a side-effect\n"); }

    configA = 1;
    multiverse_commit();
    empty_variant();
    if (constant()) { printf("Just to have a side-effect\n"); }

    return 0;
}
