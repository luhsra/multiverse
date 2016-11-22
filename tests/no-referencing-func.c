/*
 * Nothing should happen if we commit a multiverse variable that is not (yet)
 * referenced by any function.
 */

#include <stdio.h>
#include "multiverse.h"
#include "testsuite.h"

__attribute__((multiverse)) unsigned char config;


int main(int argc, char **argv)
{
    multiverse_init();

    int x = 0;
    config = 1;

    x = multiverse_commit_refs(&config);
    assert(x == 0);

    return 0;
}
