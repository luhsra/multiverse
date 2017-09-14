/*
 * A very simple test case with multiversed function pointers.
 * Assign functions to a function pointer variable, commit and call.
 *
 * Note that this is different from the function-pointer test case.
 * The function pointer itself gets 'multiversed' here.
 */

#include <stdio.h>
#include "multiverse.h"
#include "testsuite.h"


int foo1(void) {
    return 23;
}

int foo2(void) {
    return 42;
}

__attribute__((multiverse)) int (*fp)(void);


int main(int argc, char **argv)
{
    multiverse_init();

    fp = foo1;
    multiverse_commit_fn(&fp);
    assert(fp() == 23);

    fp = foo2;
    multiverse_commit_fn(&fp);
    assert(fp() == 42);

    return 0;
}
