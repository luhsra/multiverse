/*
 * This test case should exemplify and test the "tracked" semantics of
 * multiverse variables.  Marking a multiverse variable var as tracked via
 * '__attribute__((multiverse("tracked"))) TYPE var' adds the semantics that var
 * can also be unbound while committing, such that referencing functions will
 * not be specialized with var's current value.  The vector of possible values
 * is thus extended by one (i.e., unbound).
 *
 * The binding state of a tracked variable can be altered via the
 * multiverse_bind(void* var_location, int state) interface, where var_location
 * points to the specific variable and state defines the current binding state:
 *      -  1 if variable should be bound
 *      -  0 if variable should be unbound
 *      - -1 to query the binding state
 */

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
