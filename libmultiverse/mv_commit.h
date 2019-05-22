#ifndef __MULTIVERSE_COMMIT_H
#define __MULTIVERSE_COMMIT_H

typedef enum  {
    PP_TYPE_INVALID,
    PP_TYPE_X86_CALL,
    PP_TYPE_X86_CALL_INDIRECT,
    PP_TYPE_X86_JUMP,
} mv_info_patchpoint_type;

struct mv_patchpoint {
    struct mv_patchpoint *next;
    struct mv_info_fn *function;
    void *location;                // == callsite call_label
    mv_info_patchpoint_type type;

    // Here we swap in the code, we overwrite
    unsigned char swapspace[6];
};

/**
 * This method patches functions which already have been committed before the module was loaded.
 * If this is the case, then the patch points contain all the data needed for the patching process.
 */
int multiverse_mod_patch_committed_functions(struct mv_patchpoint *patchpoints);

/**
 * This method takes a list of function references and commits every function in it.
 * 
 * This list is created upon the load of a module.
 * The module may reference multiversed-variables in its functions which don't belong to the module itself.
 * If these variables were commited we need to commit the functions defined in the module as well.
 */
void multiverse_mod_commit_functions(struct mv_info_fn_ref *func_ref_list);


#endif
