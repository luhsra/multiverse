#include <stdio.h>
#include "mv_info.h"
#include <assert.h>

extern void foo();

static struct mv_info *mv_information;

void __multiverse_init(struct mv_info *info) {
    info->next = mv_information;
    mv_information = info;
}

static struct mv_info_var *
multiverse_resolve_var_info(void  *mv_variable) {
    struct mv_info *info = mv_information;
    while(info) {
        for (unsigned i = 0; i < info->n_variables; ++i) {
            struct mv_info_var *var = &info->variables[i];
            if (var->variable == mv_variable) return var;
        }
    }
    return NULL;
}

void multiverse_init() {
    struct mv_info *info = mv_information;

    while (info) {
        fprintf(stderr, "got mv_info %p, version: %d\n", info, info->version);
        fprintf(stderr, "%d functions were multiversed\n", info->n_functions);
        for (unsigned i = 0; i < info->n_functions; ++i) {
            struct mv_info_fn * fn = &info->functions[i];
            fprintf(stderr, "  %s %p, %d variants\n", fn->name,
                    fn->original_function,
                    fn->n_mv_functions);
            for (unsigned j = 0; j < fn->n_mv_functions; j++) {
                struct mv_info_mvfn * mvfn = &fn->mv_functions[j];
                void (* mv_func)() = mvfn->mv_function;
                // Execute function mv_func();
                fprintf(stderr, "    %p (vars %d)\n",
                        mvfn->mv_function, mvfn->n_assignments);
                for (unsigned x = 0; x < mvfn->n_assignments; x++) {
                    // IMPORTANT: Setup variable pointer
                    struct mv_info_assignment *assign = &mvfn->assignments[x];
                    struct mv_info_var * var_info = multiverse_resolve_var_info(assign->variable);
                    assert(var_info != NULL);
                    assign->variable = var_info;

                    assert(assign->lower_bound <= assign->upper_bound);

                    fprintf(stderr, "      %s in [%d, %d]\n",
                            assign->variable->name,
                            assign->lower_bound,
                            assign->upper_bound);
                }

            }
        }
        fprintf(stderr, "%d variables were multiversed\n", info->n_variables);
        for (unsigned i = 0; i < info->n_variables; ++i) {
            struct mv_info_var *var = &info->variables[i];
            fprintf(stderr, "  %s %p (width %d)\n", var->name,
                    var->variable,
                    var->variable_width);
        }
        // mv_info from next file
        info = info->next;
    }
}
