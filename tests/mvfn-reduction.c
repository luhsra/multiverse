/*
 * If multiple mvfn descriptors point to the same function body, we can merge
 * their respective descriptors, if their assignment maps are compatible.  This
 * way we reduce the memory cost for descriptors and the search for a fitting
 * mvfn descriptor.  The employed merge algorithm is very simplistic and is
 * currenlty only fitted for boolean assignment maps.
 *
 * TODO: The test case fails with "Assertion `desc_count(&two) == 3' failed."
 *       There're actually 4 descriptors.  Let's check why.
 */

#include <stdio.h>
#include "multiverse.h"
#include "testsuite.h"

typedef enum {true, false} bool;

__attribute__((multiverse)) bool conf_a;
__attribute__((multiverse)) bool conf_b;
__attribute__((multiverse)) bool conf_c;


int __attribute((multiverse)) two()
{
    if (conf_a && conf_b) {
        printf("Do some very complex stuff\n");
        return 1;
    } else {
        return 0;
    }
}


int __attribute((multiverse)) xor()
{
    return conf_a ^ conf_b && conf_c;
}


int desc_count(void *function) {
    struct mv_info_fn *fn = multiverse_info_fn(function);
    assert(fn);
    return fn->n_mv_functions;
}


int body_count(void *function) {
    struct mv_info_fn *fn = multiverse_info_fn(function);
    assert(fn);
    void *bodies[fn->n_mv_functions];
    for (unsigned i = 0; i < fn->n_mv_functions; i++) {
        bodies[i] = fn->mv_functions[i].function_body;
    }
    int count = 0;
    for (unsigned i = 0; i < fn->n_mv_functions; i++) {
        if (!bodies[i]) continue;
        count ++;
        void *body = bodies[i];
        for (unsigned j = i; j < fn->n_mv_functions; j++) {
            if (bodies[j] == body)
                bodies[j] = NULL;
        }
    }
    return count;
}


int main(int argc, char **argv)
{
    multiverse_init();

    multiverse_dump_info(stderr);

    // For two() we need two function bodies (one is returning 0, the
    // other is printing stuff and then returning 1). Nevertheless, we
    // need three guarding descriptors.
    printf("desc count = %d\n", desc_count(&two));
    assert(desc_count(&two) == 3);
    assert(body_count(&two) == 2);
    // Functional test
    for (unsigned A = 0; A <= 1; A++) {
        for (unsigned B = 0; B <= 1; B++) {
            conf_a = A; conf_b = B;
            multiverse_commit_fn(&two);
            assert(multiverse_is_committed(&two));
            printf("%d && %d = %d\n", conf_a, conf_b, two());
            assert(two() == (conf_a && conf_b));
        }

    }

    // xor() also has only 2 bodies, less than 6 descriptor
    assert(desc_count(&xor) <= 6);
    assert(body_count(&xor) == 2);
    // Functional test
    for (conf_a = 0; conf_a <= 1; conf_a++) {
        for (conf_b = 0; conf_b <= 1; conf_b++) {
            for (conf_c = 0; conf_c <= 1; conf_c++) {
                multiverse_commit_fn(&xor);
                assert(multiverse_is_committed(&xor));
                printf("%d ^ %d && %d = %d\n", conf_a, conf_b, conf_c, xor());
                assert(xor() == (conf_a ^ conf_b && conf_c));
            }
        }
    }


    return 0;
}
