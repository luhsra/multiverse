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


struct mv_info_var *__start___multiverse_var_ptr = &__start___multiverse_var_;
struct mv_info_var *__stop___multiverse_var_ptr = &__stop___multiverse_var_;

struct mv_info_fn *__start___multiverse_fn_ptr = &__start___multiverse_fn_;
struct mv_info_fn *__stop___multiverse_fn_ptr = &__stop___multiverse_fn_;

struct mv_info_callsite *__start___multiverse_callsite_ptr = &__start___multiverse_callsite_;
struct mv_info_callsite *__stop___multiverse_callsite_ptr = &__stop___multiverse_callsite_;

// Global multiverse lists
extern struct mv_info_var_xt *mv_info_var_xt_list;
extern struct mv_info_fn_xt *mv_info_fn_xt_list;
extern struct mv_info_callsite_xt *mv_info_callsite_xt_list;


struct mv_info_var *
multiverse_info_var(void  *variable_location) {
    struct mv_info_var *var;
    for (var = __start___multiverse_var_ptr; var < __stop___multiverse_var_ptr; var++) {
        if (var->variable_location == variable_location) return var;
    }
    return NULL;
}

/**
 * Searches the variable which lies at variable_location in the modules var section.
 * It is however possible that the module used an external defined variable
 * which results in the variable_location being placed not inside this modules var section,
 * but rather in the var section of the object file which defines the extern variable.
 * 
 * Therefor we first search inside the modules own var section and after that 
 * we iterate over every other variable definition we already registed in our global list.
 * 
 * CAUTION:
 * It may still happen that the object which defines the extern variable is not already loaded
 * and therefor not registered in our global list. Then this method will fail...
 */
struct mv_info_var *multiverse_info_var_module(struct mv_info_var *start, struct mv_info_var *stop, void *variable_location){
    struct mv_info_var *var;
    struct mv_info_var_xt *global_var_list_iter;

    /* Search in own var section. */
    for (var = start; var < stop; var++) {
        if (var->variable_location == variable_location) return var;
    }

    /* Loop through all variable extensions. */
    for(global_var_list_iter = mv_info_var_xt_list; global_var_list_iter != NULL; global_var_list_iter = global_var_list_iter->next) {
        unsigned int i;
        /* Loop through all variables in the current extension. */
        for(i = 0; i < global_var_list_iter->mv_info_var_len; i++){
            struct mv_info_var *current_mv_info_var = global_var_list_iter->mv_info_var[i];

            if(current_mv_info_var->variable_location == variable_location) return current_mv_info_var;
        }
    }

    return NULL;
}

struct mv_info_fn *
multiverse_info_fn(void  *function_body) {
    struct mv_info_fn *fn;
    for (fn = __start___multiverse_fn_ptr; fn < __stop___multiverse_fn_ptr; fn++) {
        if (fn->function_body == function_body) return fn;
    }
    return NULL;
}

/**
 * Searches the fn using the function_body address in the modules fn section.
 * It is however possible that the module used an external defined function
 * which results in function_body being placed not inside this modules fn section,
 * but rather in the fn section of the object file which defines the extern function.
 * 
 * Therefor we first search inside the modules own fn section and after that 
 * we iterate over every other fn definition we already registed in our global list.
 * 
 * CAUTION:
 * It may still happen that the object which defines the extern function is not already loaded
 * and therefor not registered in our global list. Then this method will fail...
 */
struct mv_info_fn *
multiverse_info_fn_module(struct mv_info_fn *start, struct mv_info_fn *stop, void  *function_body) {
    struct mv_info_fn *fn;
    struct mv_info_fn_xt *global_fn_list_iter;

    /* Search in own fn section. */
    for (fn = start; fn < stop; fn++) {
        if (fn->function_body == function_body) return fn;
    }

    /* Loop through all fn extensions. */
    for(global_fn_list_iter = mv_info_fn_xt_list; global_fn_list_iter != NULL; global_fn_list_iter = global_fn_list_iter->next) {
        unsigned int i;
        /* Loop through all fn in the current extension. */
        for(i = 0; i < global_fn_list_iter->mv_info_fn_len; i++){
            struct mv_info_fn *current_mv_info_fn = global_fn_list_iter->mv_info_fn[i];

            if(current_mv_info_fn->function_body == function_body) return current_mv_info_fn;
        }
    }

    return NULL;
}

static
int mv_info_fn_patchpoint_append(struct mv_info_fn *fn, struct mv_patchpoint pp){
    struct mv_patchpoint **p = &fn->patchpoints_head;

    while (*p != NULL)
        p = &(*p)->next;

    *p = multiverse_os_malloc(sizeof(struct mv_patchpoint));
    if (*p == NULL)
        return -1; // FIXME: Error

    **p = pp;
    (*p)->next = NULL;

    return 0;
}

static
int mv_info_var_fn_append(struct mv_info_var *var, struct mv_info_fn *fn) {
    struct mv_info_fn_ref **f = &var->functions_head;

    while (*f != NULL)
        f = &(*f)->next;

    *f = multiverse_os_malloc(sizeof(struct mv_info_fn_ref));
    if (*f == NULL)
        return -1; // FIXME: Error

    (*f)->next = NULL;
    (*f)->fn = fn;

    return 0;
}

int multiverse_init() {
    struct mv_info_fn *fn;
    struct mv_info_callsite *callsite;

    // Step 2: Connect all the moving parts from all compilation units
    //         and fill the runtime data.
    for (fn = __start___multiverse_fn_ptr; fn < __stop___multiverse_fn_ptr; fn++) {
        int k;

        // First we install a patchpoint for the beginning of our function body
        struct mv_patchpoint pp;
        multiverse_arch_decode_function(fn, &pp);
        if (pp.type == PP_TYPE_INVALID) continue;

        if (fn->n_mv_functions != -1) {
            // Only append "self" patchpoint if fn describes a function and
            // not a function pointer
            mv_info_fn_patchpoint_append(fn, pp);
        }

        for (k = 0; k < fn->n_mv_functions; k++) {
            unsigned x;

            struct mv_info_mvfn * mvfn = &fn->mv_functions[k];
            // Let's see if we can extract further information
            // for our multiverse function, like: constant return value.
            multiverse_arch_decode_mvfn_body(mvfn);

            for (x = 0; x < mvfn->n_assignments; x++) {
                int found;
                struct mv_info_fn_ref *fref;

                // IMPORTANT: Setup variable pointer
                struct mv_info_assignment *assign = &mvfn->assignments[x];
                struct mv_info_var* fvar = multiverse_info_var(assign->variable.location);

                MV_ASSERT(fvar != NULL);
                assign->variable.info = fvar;

                MV_ASSERT(assign->lower_bound <= assign->upper_bound);

                // Add function to list of associated functions of variable
                // if not yet present.
                found = 0;
                for (fref = fvar->functions_head; fref != NULL; fref = fref->next) {
                    if (fref->fn == fn) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    int ret = mv_info_var_fn_append(fvar, fn);
                    if (ret != 0) return ret;
                }
            }
        }
    }

    for (callsite = __start___multiverse_callsite_ptr;
         callsite < __stop___multiverse_callsite_ptr; callsite++) {
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

int multiverse_init_module(
    struct mv_info_var *mod_var_start, struct mv_info_var *mod_var_stop,
    struct mv_info_fn *mod_fn_start, struct mv_info_fn *mod_fn_stop,
    struct mv_info_callsite *mod_callsite_start, struct mv_info_callsite *mod_callsite_stop
) {
    struct mv_info_fn *fn;
    struct mv_info_callsite *callsite;

    /*multiverse_os_print("Mod_var_start: 0x%llx\n", (unsigned long long)mod_var_start);
    multiverse_os_print("Mod_var_stop: 0x%llx\n", (unsigned long long)mod_var_stop);
    multiverse_os_print("Mod_fn_start: 0x%llx\n", (unsigned long long)mod_fn_start);
    multiverse_os_print("Mod_fn_stop: 0x%llx\n", (unsigned long long)mod_fn_stop);
    multiverse_os_print("Mod_callsite_start: 0x%llx\n", (unsigned long long)mod_callsite_start);
    multiverse_os_print("Mod_callsite_stop: 0x%llx\n", (unsigned long long)mod_callsite_stop);*/


    // Step 2: Connect all the moving parts from all compilation units
    //         and fill the runtime data.
    for (fn = mod_fn_start; fn < mod_fn_stop; fn++) {
        int k;

        // First we install a patchpoint for the beginning of our function body
        struct mv_patchpoint pp;
        multiverse_arch_decode_function(fn, &pp);
        if (pp.type == PP_TYPE_INVALID) continue;

        if (fn->n_mv_functions != -1) {
            // Only append "self" patchpoint if fn describes a function and
            // not a function pointer
            mv_info_fn_patchpoint_append(fn, pp);
        }

        for (k = 0; k < fn->n_mv_functions; k++) {
            unsigned x;

            struct mv_info_mvfn * mvfn = &fn->mv_functions[k];
            // Let's see if we can extract further information
            // for our multiverse function, like: constant return value.
            multiverse_arch_decode_mvfn_body(mvfn);

            for (x = 0; x < mvfn->n_assignments; x++) {
                int found;
                struct mv_info_fn_ref *fref;

                // IMPORTANT: Setup variable pointer
                struct mv_info_assignment *assign = &mvfn->assignments[x];
                struct mv_info_var* fvar = multiverse_info_var_module(mod_var_start, mod_var_stop, assign->variable.location);

                if(fvar == NULL){
                    multiverse_os_print("Fvar is null...\n");
                    multiverse_os_print("Assign address: 0x%llx - variable location: 0x%llx\n", (unsigned long long)assign, (unsigned long long)assign->variable.location);
                }

                MV_ASSERT(fvar != NULL);
                assign->variable.info = fvar;

                MV_ASSERT(assign->lower_bound <= assign->upper_bound);

                // Add function to list of associated functions of variable
                // if not yet present.
                found = 0;
                for (fref = fvar->functions_head; fref != NULL; fref = fref->next) {
                    if (fref->fn == fn) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    int ret = mv_info_var_fn_append(fvar, fn);
                    if (ret != 0) return ret;
                }
            }
        }
    }

    return 0;

    for (callsite = mod_callsite_start;
         callsite < mod_callsite_stop; callsite++) {
        struct mv_patchpoint pp;
        struct mv_info_fn *cfn = multiverse_info_fn_module(mod_fn_start, mod_fn_stop, callsite->function_body);

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

void multiverse_cleanup_module(
    struct mv_info_var_xt *mod_var_xt,
    struct mv_info_fn_xt *mod_fn_xt,
    struct mv_info_callsite_xt *mod_callsite_xt
) {
    // Revoke patchpoint-to-fn-append (mv_info_fn_patchpoint_append).

    // Revoke fn-to-var-append (mv_info_var_fn_append).

}

void multiverse_dump_info(void) {
    /* struct mv_info_var *var; */
    struct mv_info_fn *fn;
    struct mv_info_fn_xt *fn_xt_iter;
    size_t mv_info_fn_xt_index;

    /* TODO */
    /* multiverse_os_print("mv_info %p, version: %d", info, info->version); */
    /* multiverse_os_print(", %d functions multiversed\n", info->n_functions); */

    for(fn_xt_iter = mv_info_fn_xt_list; fn_xt_iter != NULL; fn_xt_iter = fn_xt_iter->next) {
        multiverse_os_print("Name: %s\n", fn_xt_iter->xt_name);

        for (mv_info_fn_xt_index = 0; mv_info_fn_xt_index < fn_xt_iter->mv_info_fn_len; mv_info_fn_xt_index++) {
            int k;
            fn = fn_xt_iter->mv_info_fn[mv_info_fn_xt_index];
            multiverse_os_print("  fn: %s 0x%llx, %d variants\n", /*", %d patchpoints(s)\n",*/
                                fn->name,
                                (unsigned long long)fn->function_body,
                                fn->n_mv_functions
                                /*fn->extra->n_patchpoints*/);
            for (k = 0; k < fn->n_mv_functions; k++) {
                unsigned x;
                struct mv_info_mvfn * mvfn = &fn->mv_functions[k];
                // Execute function mv_func();
                multiverse_os_print("    mvfn: 0x%llx (vars %d)",
                                    (unsigned long long)mvfn->function_body, mvfn->n_assignments);
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
    }

    /* TDOD */
    /* multiverse_os_print("%d variables were multiversed\n", info->n_variables); */
    /* for (var = __start___multiverse_var_ptr; var < __stop___multiverse_var_ptr; var++) { */
    /*     multiverse_os_print("  var: %s %p (width %d, tracked:%d, signed:%d), %d functions\n", */
    /*                         var->name, */
    /*                         var->variable_location, */
    /*                         var->variable_width, */
    /*                         var->flag_tracked, */
    /*                         var->flag_signed, */
    /*                         var->n_functions); */
    /* } */
}
