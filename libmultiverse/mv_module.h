#ifndef __MV_MODULE_H
#define __MV_MODULE_H

#include "multiverse.h"
#include <linux/export.h>

// Global multiverse lists
extern struct mv_info_var_xt *mv_info_var_xt_list;
extern struct mv_info_fn_xt *mv_info_fn_xt_list;
extern struct mv_info_callsite_xt *mv_info_callsite_xt_list;

/**
 * Searches the variable which lies at variable_location in the modules var section.
 * It is however possible that the module used an external defined variable
 * which results in the variable_location being placed not inside this modules var section,
 * but rather in the var section of the object file which defines the extern variable.
 * 
 * Therefor we first search inside the modules own var section and after that 
 * we iterate over every other variable definition we already registed in our global list.
 */
struct mv_info_var *multiverse_info_var_find(struct mv_info_var *start, struct mv_info_var *stop, void *variable_location);

/**
 * Searches the fn using the function_body address in the modules fn section.
 * It is however possible that the module used an external defined function
 * which results in function_body being placed not inside this modules fn section,
 * but rather in the fn section of the object file which defines the extern function.
 * 
 * Therefor we first search inside the modules own fn section and after that 
 * we iterate over every other fn definition we already registed in our global list.
 */
struct mv_info_fn *multiverse_info_fn_find(struct mv_info_fn *start, struct mv_info_fn *stop, void  *function_body);

/**
 * Registers the multiversed-module in the global multiverse context.
 * It basically mimics the logic of multiverse_init() and saves some additionaly information about patch points.
 */
int multiverse_init_module(
    struct mv_info_var *mod_var_start, struct mv_info_var *mod_var_stop,
    struct mv_info_fn *mod_fn_start, struct mv_info_fn *mod_fn_stop,
    struct mv_info_callsite *mod_callsite_start, struct mv_info_callsite *mod_callsite_stop
);

/**
 * Revokes the registration process of the multiversed-module.
 */
void multiverse_cleanup_module(void *mod_addr);

EXPORT_SYMBOL(multiverse_init_module);
EXPORT_SYMBOL(multiverse_cleanup_module);

#endif