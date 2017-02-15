/*
 * A user can explicitly specify the vector of values via the "values" argument
 * when declaring a multiverse variable.
 *
 * The test case below exemplifies the usage of the value argument.
 */

#include "multiverse.h"
#include "testsuite.h"

__attribute__((multiverse(("values", 1, 3, 5)))) unsigned conf;


int __attribute__((multiverse)) func() {
    return conf;
}

int main(int argc, char **argv)
{
    multiverse_init();

    multiverse_dump_info(stderr);

    conf = 0;
    assert(func() == 0);

    conf = 1;
    multiverse_commit_refs(&conf);
    conf = 0; assert(func() == 1);

    conf = 3;
    multiverse_commit_refs(&conf);
    conf = 0; assert(func() == 3);

    conf = 5;
    multiverse_commit_refs(&conf);
    conf = 0; assert(func() == 5);

    conf = 42;
    multiverse_commit_refs(&conf);  // fall back to generic version
    conf = 13; assert(func() == 13);

    return 0;
}
