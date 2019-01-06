#ifndef __MULTIVERSE_TESTSUITE_H
#define __MULTIVERSE_TESTSUITE_H

#include <assert.h>
#include <stdlib.h>

static __attribute__((unused)) int desc_count(void *function) {
    struct mv_info_fn *fn = multiverse_info_fn(function);
    assert(fn);
    return fn->n_mv_functions;
}


static __attribute__((unused)) int body_count(void *function) {
    struct mv_info_fn *fn = multiverse_info_fn(function);
    assert(fn);
    void *bodies[fn->n_mv_functions];
    for (int i = 0; i < fn->n_mv_functions; i++) {
        bodies[i] = fn->mv_functions[i].function_body;
    }
    int count = 0;
    for (int i = 0; i < fn->n_mv_functions; i++) {
        if (!bodies[i]) continue;
        count ++;
        void *body = bodies[i];
        for (int j = i; j < fn->n_mv_functions; j++) {
            if (bodies[j] == body)
                bodies[j] = NULL;
        }
    }
    return count;
}


#endif
