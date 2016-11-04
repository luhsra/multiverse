#include <stdio.h>
#include "mv_info.h"

extern void foo();

void __multiverse_init(struct mv_info *info) {
    fprintf(stderr, "got mv_info %p, version: %d\n", info, info->version);
    fprintf(stderr, "%d functions were multiversed\n", info->n_functions);
    for (unsigned i = 0; i < info->n_functions; ++i) {
        struct mv_info_fn * fn = info->functions[i];
        fprintf(stderr, "%p %s %p %p\n", fn, fn->name,
                fn->original_function, foo);
        void (*bar)() = (void (*)())fn->original_function;
        bar();
    }

}
