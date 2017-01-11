#ifndef __GCC_PLUGIN_MULTIVERSE_H
#define __GCC_PLUGIN_MULTIVERSE_H

#include <stdint.h>
#include <string>
#include <vector>
#include <list>
#include <set>

#include "gcc-common.h"

#define MV_VERSION 42

typedef unsigned HOST_WIDE_INT mv_value_t;



struct multiverse_context {
    // data types used for the descriptors
    tree info_type, info_ptr_type;
    tree fn_type, fn_ptr_type;
    tree var_type, var_ptr_type;
    tree mvfn_type, mvfn_ptr_type;
    tree assignment_type, assignment_ptr_type;
    tree callsite_type, callsite_ptr_type;

    struct variable_t {
        tree var_decl;
        bool tracked;
        std::set<mv_value_t> values; // Come from the attribute
    };

    struct var_assign_t {
        variable_t * variable;
        const char * label;
        unsigned lower_limit;
        unsigned upper_limit;
    };

    typedef std::vector<var_assign_t> var_assign_vector_t;

    struct mvfn_t {
        tree mvfn_decl;
        var_assign_vector_t assignments;

        void dump(FILE *out) {
            fprintf(out, "mvfn:%s[", IDENTIFIER_POINTER(DECL_NAME(mvfn_decl)));
            for (auto & a : assignments) {
                fprintf(out, "%s=[%d,%d],",
                        IDENTIFIER_POINTER(DECL_NAME(a.variable->var_decl)),
                        a.lower_limit, a.upper_limit);

            }
        }
    };

    struct func_t {
        tree fn_decl;			 /* the function decl */
        std::list<mvfn_t> mv_functions;
    };

    struct callsite_t {
        tree fn_decl;			 /* the function decl */
        tree callsite_label;
    };

    // information
    std::list<func_t> functions;
    std::list<callsite_t> callsites;
    std::list<variable_t> variables;

    variable_t & add_variable(tree variable) {
        variable_t var;
        var.var_decl = variable;
        var.tracked = false;
        variables.push_back(var);
        return variables.back();
    }

    variable_t * get_variable(tree var_decl) {
        for (auto & x : variables) {
            if (x.var_decl == var_decl) return &x;
        }
        return nullptr;
    }

    func_t * get_func(tree func_decl) {
        for (auto & x : functions) {
            if (x.fn_decl == func_decl) return &x;
        }
        return nullptr;
    }

};

/*
 * The multiverse generator is used to capture all dimensions (variables) and
 * their values for a specific multiverse function.  The variables, and the
 * values are scored and sorted according to their score. One can give a maximal
 * number of elements the generator should emit. If a variable assignment never
 * changes during the generated sequence, the dimension is not multiversed.
 */
struct multiverse_variant_generator {
    typedef multiverse_context::variable_t variable_t;
    typedef multiverse_context::var_assign_t var_assign_t;
    typedef multiverse_context::var_assign_vector_t var_assign_vector_t;

private:
    std::vector<std::pair<variable_t*, var_assign_vector_t> > variables;
    std::vector<unsigned>  state;

    unsigned element_count;
    unsigned maximal_elements;
    unsigned skip_dimensions;

public:
    void add_variable(variable_t *);
    void add_variable_value(variable_t *, const char *label, mv_value_t value);

    void start(int maximal_elements = -1);
    bool end_p();
    var_assign_vector_t next();
};

// In mv-info.cc
void mv_info_init(void *event_data, void *data);
void mv_info_finish(void *event_data, void *data);

#endif
