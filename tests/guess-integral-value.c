#include <stdio.h>
#include "multiverse.h"
#include "testsuite.h"

__attribute__((multiverse)) unsigned char a;

int __attribute((multiverse)) func()
{
    if (a == 4) {
        printf("Do some very complex stuff\n");
        return 1;
    } else if (a == 35) {
        return 0;
    }
    return -1;
}


int main(int argc, char **argv)
{
    multiverse_init();

    multiverse_dump_info(stderr);

    a = 4; multiverse_commit_fn(&func);
    assert(multiverse_is_committed(&func));
    assert(func() == 1);

    a = 35; multiverse_commit_fn(&func);
    assert(multiverse_is_committed(&func));
    assert(func() == 0);

    for (unsigned i = 0; i <= 1; i++) {
        a = 0; multiverse_commit_fn(&func);
        assert(!multiverse_is_committed(&func));
        assert(func() == -1);
    }

    return 0;
}
