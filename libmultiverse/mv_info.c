#include <stdio.h>
#include "multiverse.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include "mv_commit.h"


struct mv_info *mv_information;

/* This function is called from the constructors in each object file
   with a multiverse function */
void __multiverse_init(struct mv_info *info) {
    info->next = mv_information;
    mv_information = info;
}

struct mv_info_var *
multiverse_info_var(void  *variable_location) {
    struct mv_info *info = mv_information;
    for (struct mv_info * info = mv_information; info; info = info->next) {
        for (unsigned i = 0; i < info->n_variables; ++i) {
            struct mv_info_var *var = &info->variables[i];
            if (var->variable_location == variable_location) return var;
        }
    }
    return NULL;
}

struct mv_info_fn *
multiverse_info_fn(void  *function_body) {
    for (struct mv_info * info = mv_information; info; info = info->next) {
        for (unsigned i = 0; i < info->n_functions; ++i) {
            struct mv_info_fn *fn = &info->functions[i];
            if (fn->function_body == function_body) return fn;
        }
    }
    return NULL;
}

static
int mv_info_fn_patchpoint_append(struct mv_info_fn *fn, struct mv_patchpoint pp){
    int n = fn->extra->n_patchpoints;
    fn->extra->patchpoints = realloc(fn->extra->patchpoints,
                                     (n + 1) * sizeof(struct mv_patchpoint));
    if (!fn->extra->patchpoints) return -1; //FIXME: Error
    fn->extra->patchpoints[fn->extra->n_patchpoints++] = pp;
    return 0;
}

int multiverse_init() {
    struct mv_info *info;

    /* Step 1: Set up all extra datastructures */
    for (info = mv_information; info; info = info->next) {
        for (unsigned i = 0; i < info->n_functions; ++i) {
            struct mv_info_fn * fn = &info->functions[i];
            fn->extra = calloc(1, sizeof(struct mv_info_fn_extra));
            if (!fn->extra) return -1;
        }
        for (unsigned i = 0; i < info->n_variables; ++i) {
            struct mv_info_var *var = &info->variables[i];
            var->extra = calloc(1, sizeof(struct mv_info_var_extra));
            if (!var->extra) return -1;
        }
    }

    // Step 2: Connect all the moving parts form all compilation units
    //         and fill the extra data.
    for (info = mv_information; info; info = info->next) {
        for (unsigned i = 0; i < info->n_functions; ++i) {
            struct mv_info_fn * fn = &info->functions[i];

            // First we install a patchpoint for the beginning of our function body
            struct mv_patchpoint pp;
            pp.type = PP_TYPE_X86_JUMPQ;
            pp.function = fn;
            pp.location = fn->function_body;
            mv_info_fn_patchpoint_append(fn, pp);

            for (unsigned j = 0; j < fn->n_mv_functions; j++) {
                struct mv_info_mvfn * mvfn = &fn->mv_functions[j];
                for (unsigned x = 0; x < mvfn->n_assignments; x++) {
                    // IMPORTANT: Setup variable pointer
                    struct mv_info_assignment *assign = &mvfn->assignments[x];
                    struct mv_info_var * var = multiverse_info_var(assign->variable);
                    assert(var != NULL);
                    assign->variable = var;

                    assert(assign->lower_bound <= assign->upper_bound);
                    // Add function to list of associated functions of
                    // variable, if not yet present
                    int found = 0;
                    for (unsigned int idx = 0; idx < var->extra->n_functions; idx++) {
                        if (var->extra->functions[idx] == fn) {
                            found = 1;
                            break;
                        }
                    }
                    if (!found) {
                        int n = var->extra->n_functions;
                        var->extra->functions = realloc(var->extra->functions,
                                                        (n + 1) * sizeof(struct mv_info_fn *));
                        if (!var->extra->functions) return -1;
                        var->extra->functions[var->extra->n_functions++] = fn;
                    }
                }

            }


        }

        for (unsigned i = 0; i < info->n_callsites; ++i) {
            struct mv_info_callsite *cs = &info->callsites[i];
            struct mv_info_fn *fn = multiverse_info_fn(cs->function_body);
            assert(fn && "Function from Callsite could not be resovled");

            // Try to find an x86 callq (e8 <offset>
            unsigned char *p = cs->call_label;
            void * addr = p + *(int*)(p + 1) + 5;
            if (*p == 0xe8 && addr == fn->function_body) {
                // Append patchpoint for callq
                struct mv_patchpoint pp;
                pp.type = PP_TYPE_X86_CALLQ;
                pp.function = fn;
                pp.location = p;

                mv_info_fn_patchpoint_append(fn, pp);
            } else {
                fprintf(stderr, "Could not decode callsite at %p for %s [%x, %x, %x, %x, %x]=%lx\n", p,
                        fn->name, p[0], p[1], p[2], p[3], p[4], addr);
            }
        }
    }
}

void multiverse_dump_info(FILE *out) {
    for (struct mv_info *info = mv_information; info; info = info->next) {
        fprintf(out, "mv_info %p, version: %d", info, info->version);
        fprintf(out, ", %d functions multiversed\n", info->n_functions);
        for (unsigned i = 0; i < info->n_functions; ++i) {
            struct mv_info_fn * fn = &info->functions[i];
            fprintf(out, "  fn: %s %p, %d variants, %d patchpoints(s)\n", fn->name,
                    fn->function_body,
                    fn->n_mv_functions,
                    fn->extra->n_patchpoints);
            for (unsigned j = 0; j < fn->n_mv_functions; j++) {
                struct mv_info_mvfn * mvfn = &fn->mv_functions[j];
                // Execute function mv_func();
                fprintf(out, "    mvfn: %p (vars %d)\n",
                        mvfn->function_body, mvfn->n_assignments);
                for (unsigned x = 0; x < mvfn->n_assignments; x++) {
                    struct mv_info_assignment *assign = &mvfn->assignments[x];
                    fprintf(out, "      assign: %s in [%d, %d]\n",
                            assign->variable->name,
                            assign->lower_bound,
                            assign->upper_bound);
                }

            }
            for (unsigned i = 0; i < fn->extra->n_patchpoints; ++i) {
                struct mv_patchpoint *var = &fn->extra->patchpoints[i];
                fprintf(out, "    patchpoint: [%d:%p] -> %s\n",
                        var->type,
                        var->location,
                        var->function->name);
            }
        }

        fprintf(out, "%d variables were multiversed\n", info->n_variables);
        for (unsigned i = 0; i < info->n_variables; ++i) {
            struct mv_info_var *var = &info->variables[i];
            fprintf(out, "  var: %s %p (width %d), %d functions\n", var->name,
                    var->variable_location,
                    var->variable_width,
                    var->extra->n_functions);
        }
    }
}
