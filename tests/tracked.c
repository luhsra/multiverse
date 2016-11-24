#include <stdio.h>
#include <assert.h>
#include "multiverse.h"

typedef enum {true, false} bool;


__attribute__((multiverse)) bool conf_a;
__attribute__((multiverse("tracked"))) bool conf_b;

int __attribute__((multiverse)) func() {
    int ret = 0;
    if (conf_a) {
        ret += 1;
    }
    if (conf_b) {
        ret += 2;
    }
    return ret;
}

int main(int argc, char **argv)
{
    multiverse_init();

    multiverse_commit();
    assert(multiverse_is_committed(&func));

    // ----------------------------
    // Test while conf_b is unbound.
    // func() reacts to conf_b, but not to conf_a
    for (conf_a = 0; conf_a <= 1; conf_a++) {
        for (conf_b = 0; conf_b <= 1; conf_b++) {
            assert(func() == (conf_b ? 2 : 0)
                   && "conf_b was not bound correctly");
        }
    }

    // ------------------------
    // Test while conf_b is bound
    conf_a = 0, conf_b = 0;
    int x = multiverse_bind(&conf_b, 1);
    assert(x == 1 && "Bound failed");
    assert(func() == 0);

    x = multiverse_commit();
    assert(x == 1 && "The func function was not changed");


    for (conf_a = 0; conf_a <= 1; conf_a++) {
        for (conf_b = 0; conf_b <= 1; conf_b++) {
            assert(func() == 0 && "conf_b was not bound correctly");
        }
    }

    conf_a = 0, conf_b = 0;
    x = multiverse_bind(&conf_b, 0);
    assert(x == 0 && "Unbound failed");
    multiverse_commit();

    for (conf_a = 0; conf_a <= 1; conf_a++) {
        for (conf_b = 0; conf_b <= 1; conf_b++) {
            assert(func() == (conf_b ? 2 : 0)
                   && "conf_b was not bound correctly");
        }
    }


    return 0;
}
