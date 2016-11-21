#include <stdio.h>
#include "multiverse.h"
#include "testsuite.h"

__attribute__((multiverse)) unsigned char a;
__attribute__((multiverse)) unsigned char b;
__attribute__((multiverse)) unsigned char c;

int __attribute((multiverse)) two()
{
    if (a && b) {
        printf("Do some very complex stuff\n");
        return 1;
    } else {
        return 0;
    }
}

int __attribute((multiverse)) xor()
{
    return a ^ b && c;
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
    assert(desc_count(&two) == 3);
    assert(body_count(&two) == 2);
    // Functional test
    for (unsigned A = 0; A <= 1; A++) {
        for (unsigned B = 0; B <= 1; B++) {
            a = A; b = B;
            multiverse_commit_fn(&two);
            assert(multiverse_is_committed(&two));
            printf("%d && %d = %d\n", a, b, two());
            assert(two() == (a && b));
        }

    }

    // xor() also has only 2 bodies, but 5 descriptors
    assert(desc_count(&xor) < 8);
    assert(body_count(&xor) == 2);
    // Functional test
    for (a = 0; a <= 1; a++) {
        for (b = 0; b <= 1; b++) {
            for (c = 0; c <= 1; c++) {
                multiverse_commit_fn(&xor);
                assert(multiverse_is_committed(&xor));
                printf("%d ^ %d && %d = %d\n", a, b,c, xor());
                assert(xor() == (a ^ b && c));
            }
        }
    }


    return 0;
}
