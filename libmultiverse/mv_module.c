#include "mv_assert.h"
#include "platform.h"
#include "mv_module.h"
#include "mv_lists.h"
#include "mv_commit.h"
#include "arch.h"
#include <linux/slab.h>

/**
 * Copied from libmultiverse/mv_info.c
 */
static int mv_info_fn_patchpoint_append(struct mv_info_fn *fn, struct mv_patchpoint pp){
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

/**
 * Copied from libmultiverse/mv_info.c
 */
static int mv_info_var_fn_append(struct mv_info_var *var, struct mv_info_fn *fn) {
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

/**
 * Removes all patch points of functions inside the module from all multiversed-functions.
 * Otherwise there would be dangling patchpoints-pointers if the module is unloaded.
 */
static void mv_revoke_mod_fn_patchpoints(struct mv_info_callsite_xt *mod_callsite_xt) {
    struct mv_info_fn_xt *fn_xt_iter;
    unsigned int fn_index;
    unsigned int callsite_index;
    
    if(!mod_callsite_xt) return;

    /* 
        We iterate over every function we recognized in our global list 
        and remove every patchpoint from our module from these functions.

        Therefor we iterate over the callsites and check the calling locations.
    */
   for(fn_xt_iter = mv_info_fn_xt_list; fn_xt_iter != NULL; fn_xt_iter = fn_xt_iter->next) {
       for(fn_index = 0; fn_index < fn_xt_iter->mv_info_fn_len; fn_index++) {
            struct mv_info_fn *current_fn = fn_xt_iter->mv_info_fn[fn_index];

            /* Skip functions without patchpoints. Is this even possible tho? */
            if(!current_fn->patchpoints_head) continue;

            for(callsite_index = 0; callsite_index < mod_callsite_xt->mv_info_callsite_len; callsite_index++) {
                struct mv_patchpoint *pp_iter;
                struct mv_info_callsite *current_callsite = mod_callsite_xt->mv_info_callsite[callsite_index];

                /*  Loop over all patch points and check if the current callsite is in there.
                    If this is the case, free the entry and remove it from the list.
                */
                /* Check if the first callsite is the entry we tried to find. Loop otherwise. */
                if(current_fn->patchpoints_head->location == current_callsite->call_label) {
                    /* Save the next entry to set as new head. If it is NULL that doesn't matter. */
                    struct mv_patchpoint *pp_next = current_fn->patchpoints_head->next;

                    /* Freeeee. */
                    /* TODO: Freeing is based on kernel mem. Use a more generic approach... */
                    kfree(current_fn->patchpoints_head);

                    /* Reset the list head. */
                    current_fn->patchpoints_head = pp_next;
                }
                else {
                    for(pp_iter = current_fn->patchpoints_head; pp_iter->next != NULL; pp_iter = pp_iter->next) {
                        if(pp_iter->next->location == current_callsite->call_label) {
                            struct mv_patchpoint *pp_to_remove = pp_iter->next;
                            
                            /* Remove the entry from the list. */
                            pp_iter->next = pp_iter->next->next;

                            /* Freeeee. */
                            /* TODO: Freeing is based on kernel mem. Use a more generic apporach... */
                            kfree(pp_to_remove);

                            break;
                        }
                    }
                }
            }
        }
   }
}

/**
 * Removes all functions of the module from the lists of referencing-functions of all multiversed-variables.
 * Otherwise there would be dangling function-pointers if the module is unlodaded.
 */
static void mv_revoke_mod_var_fns(struct mv_info_fn_xt *mod_fn_xt) {
    struct mv_info_var_xt *var_xt_iter;
    unsigned int var_index;
    unsigned int fn_index;
    
    if(!mod_fn_xt) return;

    for(fn_index = 0; fn_index < mod_fn_xt->mv_info_fn_len; fn_index++) {
        struct mv_info_var_xt *var_xt_iter;
        struct mv_info_fn *current_fn = mod_fn_xt->mv_info_fn[fn_index];

        for(var_xt_iter = mv_info_var_xt_list; var_xt_iter != NULL; var_xt_iter = var_xt_iter->next) {
            for(var_index = 0; var_index < var_xt_iter->mv_info_var_len; var_index++) {
                struct mv_info_fn_ref *var_fn_ref_iter;
                struct mv_info_var *current_var = var_xt_iter->mv_info_var[var_index];

                if(!current_var->functions_head) continue;

                if(current_var->functions_head->fn == NULL){
                    multiverse_os_print("FUNCTIONS HEAD FN EMPTY!!\n");
                    continue;
                }

                if(current_var->functions_head->fn == current_fn) {
                    /* Save the next entry to set as new head. If it is NULL that doesn't matter. */
                    struct mv_info_fn_ref *fn_ref_next = current_var->functions_head->next;

                    /* Freeeee. */
                    /* TODO: Freeing is based on kernel mem. Use a more generic approach... */
                    kfree(current_var->functions_head);

                    /* Reset the list head. */
                    current_var->functions_head = fn_ref_next;
                }
                else {
                    for(var_fn_ref_iter = current_var->functions_head; var_fn_ref_iter->next != NULL; var_fn_ref_iter = var_fn_ref_iter->next) {
                        if(var_fn_ref_iter->next->fn == current_fn) {
                            struct mv_info_fn_ref *fn_ref_to_remove = var_fn_ref_iter->next;
                            
                            /* Remove the entry from the list. */
                            var_fn_ref_iter->next = var_fn_ref_iter->next->next;

                            /* Freeeee. */
                            /* TODO: Freeing is based on kernel mem. Use a more generic apporach... */
                            kfree(fn_ref_to_remove);

                            break;
                        }
                    }
                }
            }
        }
    }
}


struct mv_info_var *multiverse_info_var_find(struct mv_info_var *start, struct mv_info_var *stop, void *variable_location){
    struct mv_info_var *var;
    struct mv_info_var_xt *global_var_list_iter;

    /* Search in own var section if provided. */
    if(start && stop){
        for (var = start; var < stop; var++) {
            if (var->variable_location == variable_location) return var;
        }
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

struct mv_info_fn *multiverse_info_fn_find(struct mv_info_fn *start, struct mv_info_fn *stop, void  *function_body) {
    struct mv_info_fn *fn;
    struct mv_info_fn_xt *global_fn_list_iter;

    /* Search in own fn section if provided. */
    if(start && stop){
        for (fn = start; fn < stop; fn++) {
            if (fn->function_body == function_body) return fn;
        }
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

int multiverse_init_module(
    struct mv_info_var *mod_var_start, struct mv_info_var *mod_var_stop,
    struct mv_info_fn *mod_fn_start, struct mv_info_fn *mod_fn_stop,
    struct mv_info_callsite *mod_callsite_start, struct mv_info_callsite *mod_callsite_stop
) {
    struct mv_info_fn *fn;
    struct mv_info_callsite *callsite;
    struct mv_patchpoint *module_patchpoints = NULL;
    struct mv_patchpoint *current_module_patchpoint = NULL;
    /* List of all functions in the module referencing variables which are already commited. */
    struct mv_info_fn_ref *mod_funcs_ref_comm_vars = NULL;
    struct mv_info_fn_ref *current_mod_func_ref_comm_var = NULL;

    /* Lock to avoid interference with other commits. */
    multiverse_os_lock();

    // Step 2: Connect all the moving parts from all compilation units
    //         and fill the runtime data.
    if(mod_fn_start && mod_fn_stop){
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
                    struct mv_info_var* fvar = multiverse_info_var_find(mod_var_start, mod_var_stop, assign->variable.location);

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

                        /* If the function references a variable which was already commited save this function to commit it at the end. */
                        if(fvar->flag_committed){
                            struct mv_info_fn_ref *save_func_ref = multiverse_os_malloc(sizeof(struct mv_info_fn_ref));

                            save_func_ref->fn = fn;
                            save_func_ref->next = NULL;

                            if(!mod_funcs_ref_comm_vars){
                                mod_funcs_ref_comm_vars = save_func_ref;
                                current_mod_func_ref_comm_var = save_func_ref;    
                            } else {
                                current_mod_func_ref_comm_var->next = save_func_ref;
                                current_mod_func_ref_comm_var = current_mod_func_ref_comm_var->next;
                            }
                        }
                    }
                }
            }
        }
    }
    
    if(mod_callsite_start && mod_callsite_stop){
        for (callsite = mod_callsite_start; callsite < mod_callsite_stop; callsite++) {
            struct mv_patchpoint pp;
            struct mv_info_fn *cfn = multiverse_info_fn_find(mod_fn_start, mod_fn_stop, callsite->function_body);

            /* Function was not found. Perhaps there are no multiverses? */
            if (cfn == NULL) continue;

            // Try to find a patchpoint at the callsite offset
            multiverse_arch_decode_callsite(cfn, callsite->call_label, &pp);
            if (pp.type != PP_TYPE_INVALID) {
                struct mv_patchpoint *save_pp = multiverse_os_malloc(sizeof(struct mv_patchpoint));

                pp.function = cfn;
                mv_info_fn_patchpoint_append(cfn, pp);

                *save_pp = pp;
                save_pp->next = NULL;
                /* Save patch point. */
                if(!module_patchpoints) {                    
                    module_patchpoints = save_pp;
                    current_module_patchpoint = save_pp;
                } else {
                    current_module_patchpoint->next = save_pp;
                    current_module_patchpoint = current_module_patchpoint->next;
                }
            } else {
                char *p = callsite->call_label;
                multiverse_os_print("Could not decode callsite at %p for %s [%x, %x, %x, %x, %x]\n",
                                    p, cfn->name, p[0], p[1], p[2], p[3], p[4]);
            }
        }
    }

    multiverse_mod_commit_functions(mod_funcs_ref_comm_vars);
    multiverse_mod_patch_committed_functions(module_patchpoints);

    printk(KERN_INFO "Multiverse module initialized!\n");

    multiverse_os_unlock();

    return 0;
}

void multiverse_cleanup_module(void *mod_addr) {
    struct mv_info_var_xt *mod_var_xt;
    struct mv_info_fn_xt *mod_fn_xt;
    struct mv_info_callsite_xt *mod_callsite_xt;
    multiverse_os_lock();
    
    mod_var_xt = multiverse_get_mod_var_xt_entry(mod_addr);
    mod_fn_xt = multiverse_get_mod_fn_xt_entry(mod_addr);
    mod_callsite_xt = multiverse_get_mod_callsite_xt_entry(mod_addr);;

    // Revoke patchpoint-to-fn-append (mv_info_fn_patchpoint_append).
    mv_revoke_mod_fn_patchpoints(mod_callsite_xt);

    // Revoke fn-to-var-append (mv_info_var_fn_append).
    mv_revoke_mod_var_fns(mod_fn_xt);

    // Remove entries from global lists.
    multiverse_remove_var_list(mod_var_xt);
    multiverse_remove_fn_list(mod_fn_xt);
    multiverse_remove_callsite_list(mod_callsite_xt);

    multiverse_os_unlock();
}