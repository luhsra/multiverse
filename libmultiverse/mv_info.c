#include "mv_assert.h"
#include "platform.h"
#include "multiverse.h"
#include "mv_commit.h"
#include "arch.h"


struct mv_info *mv_information;

/* This function is called from the constructors in each object file
   with a multiverse function */
void __multiverse_init(struct mv_info *info) {
    info->next = mv_information;
    mv_information = info;
}

struct mv_info_var *
multiverse_info_var(void  *variable_location) {
    struct mv_info * info;
    for (info = mv_information; info; info = info->next) {
        unsigned i;
        for (i = 0; i < info->n_variables; ++i) {
            struct mv_info_var *var = &info->variables[i];
            if (var->variable_location == variable_location) return var;
        }
    }
    return NULL;
}

struct mv_info_fn *
multiverse_info_fn(void  *function_body) {
    struct mv_info * info;
    for (info = mv_information; info; info = info->next) {
        unsigned i;
        for (i = 0; i < info->n_functions; ++i) {
            struct mv_info_fn *fn = &info->functions[i];
            if (fn->function_body == function_body) return fn;
        }
    }
    return NULL;
}

static
int mv_info_fn_patchpoint_append(struct mv_info_fn *fn, struct mv_patchpoint pp){
    int n = fn->extra->n_patchpoints;
    fn->extra->patchpoints =
        multiverse_os_realloc(fn->extra->patchpoints,
                              (n + 1) * sizeof(struct mv_patchpoint));
    if (!fn->extra->patchpoints) return -1; //FIXME: Error
    fn->extra->patchpoints[fn->extra->n_patchpoints++] = pp;
    return 0;
}

int multiverse_init() {
    struct mv_info *info;

    multiverse_os_print("mv_init\n");

    /* Step 1: Set up all extra datastructures */
    for (info = mv_information; info; info = info->next) {
        unsigned i;
        for (i = 0; i < info->n_functions; ++i) {
            struct mv_info_fn * fn = &info->functions[i];
            fn->extra = multiverse_os_calloc(1, sizeof(struct mv_info_fn_extra));
            if (!fn->extra) return -1;
        }
        for (i = 0; i < info->n_variables; ++i) {
            struct mv_info_var *var = &info->variables[i];
            var->extra = multiverse_os_calloc(1, sizeof(struct mv_info_var_extra));
            if (!var->extra) return -1;

            if (var->flag_tracked)
                var->extra->bound = 0;
            else
                var->extra->bound = 1;
        }
    }

    // Step 2: Connect all the moving parts from all compilation units
    //         and fill the extra data.
    for (info = mv_information; info; info = info->next) {
        unsigned i;

        for (i = 0; i < info->n_functions; ++i) {
            unsigned j;
            struct mv_info_fn * fn = &info->functions[i];

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
                struct mv_info_mvfn_extra extra;
                multiverse_arch_decode_mvfn_body(mvfn->function_body, &extra);
                if (extra.type != MVFN_TYPE_NONE) {
                    mvfn->extra = multiverse_os_malloc(sizeof(extra));
                    MV_ASSERT(mvfn->extra != NULL);
                    *mvfn->extra = extra;
                }
                for (x = 0; x < mvfn->n_assignments; x++) {
                    int found;
                    unsigned idx;

                    // IMPORTANT: Setup variable pointer
                    struct mv_info_assignment *assign = &mvfn->assignments[x];
                    struct mv_info_var * var = multiverse_info_var(assign->variable);

                    MV_ASSERT(var != NULL);
                    assign->variable = var;

                    MV_ASSERT(assign->lower_bound <= assign->upper_bound);

                    // Add function to list of associated functions of
                    // variable, if not yet present
                    found = 0;
                    for (idx = 0; idx < var->extra->n_functions; idx++) {
                        if (var->extra->functions[idx] == fn) {
                            found = 1;
                            break;
                        }
                    }
                    if (!found) {
                        int n = var->extra->n_functions;
                        var->extra->functions =
                            multiverse_os_realloc(var->extra->functions, (n + 1)
                                                  * sizeof(struct mv_info_fn *));
                        if (!var->extra->functions) return -1;
                        var->extra->functions[var->extra->n_functions++] = fn;
                    }
                }

            }
        }

        for (i = 0; i < info->n_callsites; ++i) {
            struct mv_patchpoint pp;
            struct mv_info_callsite *cs = &info->callsites[i];
            struct mv_info_fn *fn = multiverse_info_fn(cs->function_body);

            /* Function was not found. Perhaps there are no multiverses? */
            if (fn == NULL) continue;

            // Try to find a patchpoint at the callsite offset
            multiverse_arch_decode_callsite(fn, cs->call_label, &pp);
            if (pp.type != PP_TYPE_INVALID) {
                pp.function = fn;
                mv_info_fn_patchpoint_append(fn, pp);
            } else {
                char *p = cs->call_label;
                multiverse_os_print(
                    "Could not decode callsite at %p for %s [%x, %x, %x, %x, %x]\n",
                    p, fn->name, p[0], p[1], p[2], p[3], p[4]);
            }
        }
    }
    return 0;
}

void multiverse_dump_info(void) {
    struct mv_info *info;
    for (info = mv_information; info; info = info->next) {
        unsigned i;
        multiverse_os_print("mv_info %p, version: %d", info, info->version);
        multiverse_os_print(", %d functions multiversed\n", info->n_functions);
        for (i = 0; i < info->n_functions; ++i) {
            unsigned j;
            struct mv_info_fn * fn = &info->functions[i];
            multiverse_os_print("  fn: %s %p, %d variants, %d patchpoints(s)\n",
                                fn->name,
                                fn->function_body,
                                fn->n_mv_functions,
                                fn->extra->n_patchpoints);
            for (j = 0; j < fn->n_mv_functions; j++) {
                unsigned x;
                struct mv_info_mvfn * mvfn = &fn->mv_functions[j];
                // Execute function mv_func();
                multiverse_os_print("    mvfn: %p (vars %d)",
                                    mvfn->function_body, mvfn->n_assignments);
                if (fn->extra->active_mvfn == mvfn) {
                    multiverse_os_print("<-- active");
                }
                multiverse_os_print("\n");
                for (x = 0; x < mvfn->n_assignments; x++) {
                    struct mv_info_assignment *assign = &mvfn->assignments[x];
                    multiverse_os_print("      assign: %s in [%d, %d]\n",
                                        assign->variable->name,
                                        assign->lower_bound,
                                        assign->upper_bound);
                }

            }
            for (j = 0; j < fn->extra->n_patchpoints; ++j) {
                struct mv_patchpoint *var = &fn->extra->patchpoints[j];
                multiverse_os_print("    patchpoint: [%d:%p] -> %s\n",
                                    var->type,
                                    var->location,
                                    var->function->name);
            }
        }

        multiverse_os_print("%d variables were multiversed\n", info->n_variables);
        for (i = 0; i < info->n_variables; ++i) {
            struct mv_info_var *var = &info->variables[i];
            multiverse_os_print("  var: %s %p (width %d, tracked:%d, signed:%d), %d functions\n",
                                var->name,
                                var->variable_location,
                                var->variable_width,
                                var->flag_tracked,
                                var->flag_signed,
                                var->extra->n_functions);
        }
    }
}
