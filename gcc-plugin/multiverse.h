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
    struct decl_ref_t {
    private:
        const_tree asm_name;
    public:

        decl_ref_t(tree decl)
            : asm_name(DECL_ASSEMBLER_NAME(decl)) {}

        void relink_to(decl_ref_t *other) {
            this->asm_name = other->asm_name;
        }

        tree decl() {
            symtab_node *target = symtab_node::get_for_asmname(asm_name);
            return target->decl;
        }

        const char *name() {
            return IDENTIFIER_POINTER(asm_name);
        }
    };

    struct variable_t : public decl_ref_t {
        variable_t(tree decl) : decl_ref_t(decl) {}

        std::set<mv_value_t> values; // Comes from the attribute
        bool tracked;


    };

    struct var_assign_t {
        variable_t * variable;
        const char * label;
        unsigned lower_limit;
        unsigned upper_limit;

        void dump(FILE *out) {
            fprintf(out, "%s=[%d,%d],",
                    variable->name(), lower_limit, upper_limit);
        }
    };
    typedef std::vector<var_assign_t> var_assign_vector_t;


    struct mvfn_t : public decl_ref_t {
        mvfn_t(tree decl) : decl_ref_t(decl) {}

        var_assign_vector_t assignments;

        void dump(FILE *out) {
            fprintf(out, "mvfn:%s[", this->name());
            for (auto & a : assignments) {
                a.dump(out);
            }
        }
    };

    struct func_t : public decl_ref_t {
        func_t(tree decl) : decl_ref_t(decl) {}

        std::list<mvfn_t> mv_functions;
    };

    struct callsite_t {
        tree fn_decl;                    /* the callsite function decl */
        tree callsite_label;
    };

    // information
    std::list<func_t> functions;
    std::list<callsite_t> callsites;
    std::list<variable_t> variables;

    variable_t & add_variable(tree var_decl) {
        if (variable_t * x = get_variable(var_decl)) {
            return *x;
        }

        variable_t var(var_decl);
        var.tracked = false;
        variables.push_back(var);
        return variables.back();
    }

    variable_t * get_variable(tree var_decl) {
        const_tree asm_name = DECL_ASSEMBLER_NAME(var_decl);

        for (auto & x : variables) {
            if (!strcmp(x.name(), IDENTIFIER_POINTER(asm_name))) {
                return &x;
            }
        }
        return nullptr;
    }

    func_t & add_func(tree func_decl) {
        if (func_t * x = get_func(func_decl)) {
            return *x;
        }

        func_t var(func_decl);
        functions.push_back(var);
        return functions.back();
    }


    func_t * get_func(tree func_decl) {
        const_tree asm_name = DECL_ASSEMBLER_NAME(func_decl);

        for (auto & x : functions) {
            if (!strcmp(x.name(), IDENTIFIER_POINTER(asm_name))) {
                return &x;
            }
        }
        return nullptr;
    }

};


/*
 * Contains the types for the multiverse info descriptors.
 */
struct multiverse_info_types {
    const tree fn_type, fn_ptr_type;
    const tree var_type, var_ptr_type;
    const tree mvfn_type, mvfn_ptr_type;
    const tree assignment_type, assignment_ptr_type;
    const tree callsite_type, callsite_ptr_type;

    /* Constructs all the multiverse descriptor types and returns them. */
    static multiverse_info_types build();

private:
    multiverse_info_types(tree fn_type, tree fn_ptr_type,
                          tree var_type, tree var_ptr_type,
                          tree mvfn_type, tree mvfn_ptr_type,
                          tree assignment_type,
                          tree assignment_ptr_type,
                          tree callsite_type,
                          tree callsite_ptr_type);
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
