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
    /* \brief reference to a named declaration

       We use this wrapper class to have a stable reference to a named
       GCC tree object. Due to optimizations and different phases in
       the compiler, tree references can become invalid. Therefore, we
       reference into the internal GCC structures by saving the
       assembler name.
    */
    struct decl_ref_t {
    private:
        const_tree asm_name;
    public:

        decl_ref_t(tree decl)
            : asm_name(DECL_ASSEMBLER_NAME(decl)) {}

        void relink_to(decl_ref_t *other) {
            this->asm_name = other->asm_name;
        }

        bool is_ref_to(tree decl) {
            return !strcmp(this->name(),
                           IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(decl)));
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
        variable_t(tree decl) : decl_ref_t(decl), tracked(false) {}

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



    template<class T>
    struct decl_ref_container : public std::list<T> {
        T& add(tree decl) {
            if (T * x = get(decl)) return *x;
            this->emplace_back(decl);
            return this->back();
        }

        T* get(tree decl) {
            for (auto & x : *this) {
                if (x.is_ref_to(decl)) {
                    return &x;
                }
            }
            return nullptr;
        }
    };


    // Data Members below:
    std::list<callsite_t> callsites;
    decl_ref_container<func_t> functions;
    decl_ref_container<variable_t> variables;
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
