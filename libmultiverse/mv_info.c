#include "mv_assert.h"
#include "platform.h"
#include "multiverse.h"
#include "mv_commit.h"
#include "arch.h"


extern struct mv_info_var __attribute__((weak)) __start___multiverse_var_;
extern struct mv_info_var __attribute__((weak)) __stop___multiverse_var_;

extern struct mv_info_fn __attribute__((weak)) __start___multiverse_fn_;
extern struct mv_info_fn __attribute__((weak)) __stop___multiverse_fn_;

extern struct mv_info_callsite __attribute__((weak)) __start___multiverse_callsite_;
extern struct mv_info_callsite __attribute__((weak)) __stop___multiverse_callsite_;


struct mv_info_var *
multiverse_info_var(void  *variable_location) {
    struct mv_info_var *var;
    for (var = &__start___multiverse_var_; var < &__stop___multiverse_var_; var++) {
        if (var->variable_location == variable_location) return var;
    }
    return NULL;
}

struct mv_info_fn *
multiverse_info_fn(void  *function_body) {
    struct mv_info_fn *fn;
    for (fn = &__start___multiverse_fn_; fn < &__stop___multiverse_fn_; fn++) {
        if (fn->function_body == function_body) return fn;
    }
    return NULL;
}

static
int mv_info_fn_patchpoint_append(struct mv_info_fn *fn, struct mv_patchpoint pp){
    struct mv_patchpoint **p = &fn->patchpoints_head;

    while (*p != NULL)
        p = &(*p)->next;

    *p = multiverse_os_malloc(sizeof(struct mv_patchpoint));
    if (p == NULL)
        return -1; // FIXME: Error

    **p = pp;
    (*p)->next = NULL;

    return 0;
}

int multiverse_init() {
    struct mv_info_fn *fn;
    struct mv_info_callsite *callsite;

    // Step 2: Connect all the moving parts from all compilation units
    //         and fill the runtime data.
    for (fn = &__start___multiverse_fn_; fn < &__stop___multiverse_fn_; fn++) {
        unsigned j;

        // First we install a patchpoint for the beginning of our function body
        struct mv_patchpoint pp;
        multiverse_arch_decode_function(fn, &pp);
        if (pp.type == PP_TYPE_INVALID) continue;

        if (fn->n_mv_functions > 0) {
            // Only append "self" patchpoint if fn describes a function and
            // not a function pointer
            mv_info_fn_patchpoint_append(fn, pp);
        }

        for (j = 0; j < fn->n_mv_functions; j++) {
            unsigned x;

            struct mv_info_mvfn * mvfn = &fn->mv_functions[j];
            // Let's see if we can extract further information
            // for our multiverse function, like: constant return value.
            multiverse_arch_decode_mvfn_body(mvfn);

            for (x = 0; x < mvfn->n_assignments; x++) {
                int found;
                unsigned idx;

                // IMPORTANT: Setup variable pointer
                struct mv_info_assignment *assign = &mvfn->assignments[x];
                struct mv_info_var* fvar = multiverse_info_var(assign->variable.location);

                MV_ASSERT(fvar != NULL);
                assign->variable.info = fvar;

                MV_ASSERT(assign->lower_bound <= assign->upper_bound);

                // Add function to list of associated functions of
                // variable, if not yet present
                found = 0;
                for (idx = 0; idx < fvar->n_functions; idx++) {
                    if (fvar->functions[idx] == fn) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    int n = fvar->n_functions;
                    fvar->functions =
                        multiverse_os_realloc(fvar->functions, (n + 1)
                                              * sizeof(struct mv_info_fn *));
                    if (!fvar->functions) return -1;
                    fvar->functions[fvar->n_functions++] = fn;
                }
            }

        }
    }

    for (callsite = &__start___multiverse_callsite_;
         callsite < &__stop___multiverse_callsite_; callsite++) {
        struct mv_patchpoint pp;
        struct mv_info_fn *cfn = multiverse_info_fn(callsite->function_body);

        /* Function was not found. Perhaps there are no multiverses? */
        if (cfn == NULL) continue;

        // Try to find a patchpoint at the callsite offset
        multiverse_arch_decode_callsite(cfn, callsite->call_label, &pp);
        if (pp.type != PP_TYPE_INVALID) {
            pp.function = cfn;
            mv_info_fn_patchpoint_append(cfn, pp);
        } else {
            char *p = callsite->call_label;
            multiverse_os_print("Could not decode callsite at %p for %s [%x, %x, %x, %x, %x]\n",
                                p, cfn->name, p[0], p[1], p[2], p[3], p[4]);
        }
    }
    return 0;
}

void multiverse_dump_info(void) {
    struct mv_info_var *var;
    struct mv_info_fn *fn;

    /* TODO */
    /* multiverse_os_print("mv_info %p, version: %d", info, info->version); */
    /* multiverse_os_print(", %d functions multiversed\n", info->n_functions); */

    for (fn = &__start___multiverse_fn_; fn < &__stop___multiverse_fn_; fn++) {
        unsigned j;
        multiverse_os_print("  fn: %s %p, %d variants\n", /*", %d patchpoints(s)\n",*/
                            fn->name,
                            fn->function_body,
                            fn->n_mv_functions
                            /*fn->extra->n_patchpoints*/);
        for (j = 0; j < fn->n_mv_functions; j++) {
            unsigned x;
            struct mv_info_mvfn * mvfn = &fn->mv_functions[j];
            // Execute function mv_func();
            multiverse_os_print("    mvfn: %p (vars %d)",
                                mvfn->function_body, mvfn->n_assignments);
            if (fn->active_mvfn == mvfn) {
                multiverse_os_print("<-- active");
            }
            multiverse_os_print("\n");
            for (x = 0; x < mvfn->n_assignments; x++) {
                struct mv_info_assignment *assign = &mvfn->assignments[x];
                multiverse_os_print("      assign: %s in [%d, %d]\n",
                                    assign->variable.info->name,
                                    assign->lower_bound,
                                    assign->upper_bound);
            }

        }
        /* for (j = 0; j < fn->extra->n_patchpoints; ++j) { */
        /*     struct mv_patchpoint *fvar = &fn->extra->patchpoints[j]; */
        /*     multiverse_os_print("    patchpoint: [%d:%p] -> %s\n", */
        /*                         fvar->type, */
        /*                         fvar->location, */
        /*                         fvar->function->name); */
        /* } */
    }

    /* TDOD */
    /* multiverse_os_print("%d variables were multiversed\n", info->n_variables); */
    for (var = &__start___multiverse_var_; var < &__stop___multiverse_var_; var++) {
        multiverse_os_print("  var: %s %p (width %d, tracked:%d, signed:%d), %d functions\n",
                            var->name,
                            var->variable_location,
                            var->variable_width,
                            var->flag_tracked,
                            var->flag_signed,
                            var->n_functions);
    }
}
