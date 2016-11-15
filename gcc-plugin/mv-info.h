#ifndef __GCC_MULTIVERSE_PLUGIN_INFO_H
#define __GCC_MULTIVERSE_PLUGIN_INFO_H

#include "gcc-common.h"
#include <list>

#define MV_VERSION 42

struct mv_info_assignment_data {
    tree var_decl;
    unsigned lower_limit;
    unsigned upper_limit;
};

struct mv_info_fn_data;


struct mv_info_mvfn_data {
    tree mvfn_decl;
    std::list<mv_info_assignment_data> assignments;
};


struct mv_info_fn_data {
    tree fn_decl;			 /* the function decl */
    std::list<mv_info_mvfn_data> mv_functions;
};

struct mv_info_callsite_data {
    tree fn_decl;			 /* the function decl */
    tree callsite_label;
};

struct mv_info_var_data {
    tree var_decl;			 /* the multiverse variable decl */
};


/*
 * These record types are used to pass on the information about the multiverses
 * instanciated in this compilation unit.
 */
typedef struct {
    // data types
    tree info_type, info_ptr_type;
    tree fn_type, fn_ptr_type;
    tree var_type, var_ptr_type;
    tree mvfn_type, mvfn_ptr_type;
    tree assignment_type, assignment_ptr_type;
    tree callsite_type, callsite_ptr_type;


    // information
    std::list<mv_info_var_data> variables;
    std::list<mv_info_fn_data> functions;
    std::list<mv_info_callsite_data> callsites;
} mv_info_ctx_t;

void mv_info_init(void *event_data, void *data);
void mv_info_finish(void *event_data, void *data);

#endif
