#include <stdio.h>
#include "mv_info.h"
#include <assert.h>

extern void foo();

void __multiverse_init(struct mv_info *info) {
    fprintf(stderr, "got mv_info %p, version: %d\n", info, info->version);
    fprintf(stderr, "%d functions were multiversed\n", info->n_functions);
    for (unsigned i = 0; i < info->n_functions; ++i) {
        struct mv_info_fn * fn = info->functions[i];
        fprintf(stderr, "  %s %p, %d variants\n", fn->name,
                fn->original_function,
                fn->n_mv_functions);
        for (unsigned j = 0; j < fn->n_mv_functions; j++) {
            struct mv_info_mvfn * mvfn = fn->mv_functions[j];
            assert(mvfn->function == fn);
            fprintf(stderr, "    %p (vars %d)\n",
                    mvfn->mv_function, mvfn->n_assignments);
            for (unsigned x = 0; x < mvfn->n_assignments; x++) {
                fprintf(stderr, "      %p in [%d, %d]\n",
                        mvfn->assignments[x].variable,
                        mvfn->assignments[x].lower_bound,
                        mvfn->assignments[x].upper_bound);
            }

        }
    }
}
