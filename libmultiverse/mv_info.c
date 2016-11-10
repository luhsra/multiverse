#include <stdio.h>
#include "multiverse.h"
#include <assert.h>
#include <stdint.h>

extern void foo();

static struct mv_info *mv_information;

/* This function is called from the constructors in each object file
   with a multiverse function */
void __multiverse_init(struct mv_info *info) {
    info->next = mv_information;
    mv_information = info;
}

struct mv_info_var *
multiverse_var_info(void  *variable_location) {
    struct mv_info *info = mv_information;
    while(info) {
        for (unsigned i = 0; i < info->n_variables; ++i) {
            struct mv_info_var *var = &info->variables[i];
            if (var->variable_location == variable_location) return var;
        }
    }
    return NULL;
}

struct mv_info_fn *
multiverse_fn_info(void  *function_body) {
    struct mv_info *info = mv_information;
    while(info) {
        for (unsigned i = 0; i < info->n_functions; ++i) {
            struct mv_info_fn *fn = &info->functions[i];
            if (fn->function_body == function_body) return fn;
        }
    }
    return NULL;
}

void multiverse_init() {
    struct mv_info *info = mv_information;

    while (info) {
        for (unsigned i = 0; i < info->n_functions; ++i) {
            struct mv_info_fn * fn = &info->functions[i];
            for (unsigned j = 0; j < fn->n_mv_functions; j++) {
                struct mv_info_mvfn * mvfn = &fn->mv_functions[j];
                for (unsigned x = 0; x < mvfn->n_assignments; x++) {
                    // IMPORTANT: Setup variable pointer
                    struct mv_info_assignment *assign = &mvfn->assignments[x];
                    struct mv_info_var * var_info = multiverse_var_info(assign->variable);
                    assert(var_info != NULL);
                    assign->variable = var_info;

                    assert(assign->lower_bound <= assign->upper_bound);
                }

            }
        }

        for (unsigned i = 0; i < info->n_callsites; ++i) {
            struct mv_info_callsite *var = &info->callsites[i];
            var->function = multiverse_fn_info(var->function);
            assert(var->function && "Function from Callsite could not be resovled");
        }
        // mv_info from next file
        info = info->next;
    }
}

void multiverse_dump_info(FILE *out) {
    struct mv_info *info = mv_information;

    while (info) {
        fprintf(out, "mv_info %p, version: %d", info, info->version);
        fprintf(out, ", %d functions multiversed\n", info->n_functions);
        for (unsigned i = 0; i < info->n_functions; ++i) {
            struct mv_info_fn * fn = &info->functions[i];
            fprintf(out, "  fn: %s %p, %d variants\n", fn->name,
                    fn->function_body,
                    fn->n_mv_functions);
            for (unsigned j = 0; j < fn->n_mv_functions; j++) {
                struct mv_info_mvfn * mvfn = &fn->mv_functions[j];
                // Execute function mv_func();
                fprintf(out, "    mvfn: %p (vars %d)\n",
                        mvfn->mv_function, mvfn->n_assignments);
                for (unsigned x = 0; x < mvfn->n_assignments; x++) {
                    struct mv_info_assignment *assign = &mvfn->assignments[x];
                    fprintf(out, "      assign: %s in [%d, %d]\n",
                            assign->variable->name,
                            assign->lower_bound,
                            assign->upper_bound);
                }

            }
        }
        fprintf(out, "%d variables were multiversed\n", info->n_variables);
        for (unsigned i = 0; i < info->n_variables; ++i) {
            struct mv_info_var *var = &info->variables[i];
            fprintf(out, "  var: %s %p (width %d)\n", var->name,
                    var->variable_location,
                    var->variable_width);
        }

        fprintf(out, "%d callsites were recored\n", info->n_callsites);
        for (unsigned i = 0; i < info->n_callsites; ++i) {
            struct mv_info_callsite *var = &info->callsites[i];
            fprintf(out, "  callsite: [%p,%p (%d bytes)] -> %s\n",
                    var->label_before,
                    var->label_after,
                    (intptr_t)var->label_after - (intptr_t)var->label_before,
                    var->function->name);
        }
        // mv_info from next file
        info = info->next;
    }
}
