#include <stdio.h>
#include "multiverse.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

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
    while(info) {
        for (unsigned i = 0; i < info->n_variables; ++i) {
            struct mv_info_var *var = &info->variables[i];
            if (var->variable_location == variable_location) return var;
        }
    }
    return NULL;
}

struct mv_info_fn *
multiverse_info_fn(void  *function_body) {
    struct mv_info *info = mv_information;
    while(info) {
        for (unsigned i = 0; i < info->n_functions; ++i) {
            struct mv_info_fn *fn = &info->functions[i];
            if (fn->function_body == function_body) return fn;
        }
    }
    return NULL;
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
            // Mark the variable as unitialized
            var->extra->materialized_value = MV_UNINITIALIZED_VARIABLE;
        }
    }

    // Step 2: Connect all the moving parts form all compilation units
    //         and fill the extra data.
    for (info = mv_information; info; info = info->next) {
        for (unsigned i = 0; i < info->n_functions; ++i) {
            struct mv_info_fn * fn = &info->functions[i];
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
            struct mv_info_callsite_original *cso =
                (struct mv_info_callsite_original *) cs;
            struct mv_info_fn *fn = multiverse_info_fn(cso->function_body);
            cs->function = fn;
            assert(fn && "Function from Callsite could not be resovled");

            // Try to find an x86 callq (e8 <offset>
            void * call_insn = NULL;
            int instances = 0;
            if (cso->label_after <= cso->label_before) {
                cs->type = CS_TYPE_INVALID;
                cs->call_insn = 0;
            } else {
                for (unsigned char *p = ((char *) cso->label_after) - 5; p != cso->label_before; p --) {
                    void * addr = p + *(int*)(p + 1) + 5;
                    if (*p == 0xe8 && addr == fn->function_body) {
                        call_insn = p; instances ++;
                    }
                }
                if (instances == 1) {
                    cs->type = CS_TYPE_X86_CALLQ;
                    cs->call_insn = call_insn;
                } else if (instances == 0) {
                    cs->type = CS_TYPE_NOTFOUND;
                    cs->call_insn = NULL;
                } else if (instances == 0) {
                    cs->type = CS_TYPE_INVALID;
                    cs->call_insn = NULL;
                }
            }
            /* Add variable to the list of callsites of the function */
            int n = fn->extra->n_callsites;
            fn->extra->callsites = realloc(fn->extra->callsites,
                                           (n + 1) * sizeof(struct mv_info_callsite *));
            if (!fn->extra->callsites) return -1;
            fn->extra->callsites[fn->extra->n_callsites++] = cs;
        }
    }
}

void multiverse_dump_info(FILE *out) {
    for (struct mv_info *info = mv_information; info; info = info->next) {
        fprintf(out, "mv_info %p, version: %d", info, info->version);
        fprintf(out, ", %d functions multiversed\n", info->n_functions);
        for (unsigned i = 0; i < info->n_functions; ++i) {
            struct mv_info_fn * fn = &info->functions[i];
            fprintf(out, "  fn: %s %p, %d variants, %d callsite(s)\n", fn->name,
                    fn->function_body,
                    fn->n_mv_functions,
                    fn->extra->n_callsites);
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
        }
        fprintf(out, "%d variables were multiversed\n", info->n_variables);
        for (unsigned i = 0; i < info->n_variables; ++i) {
            struct mv_info_var *var = &info->variables[i];
            fprintf(out, "  var: %s %p (width %d), %d functions\n", var->name,
                    var->variable_location,
                    var->variable_width,
                    var->extra->n_functions);
        }

        fprintf(out, "%d callsites were recored\n", info->n_callsites);
        for (unsigned i = 0; i < info->n_callsites; ++i) {
            struct mv_info_callsite *var = &info->callsites[i];
            fprintf(out, "  callsite: [%d:%p] -> %s\n",
                    var->type,
                    var->call_insn,
                    var->function->name);
        }
    }
}
