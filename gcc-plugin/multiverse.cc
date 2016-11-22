/*
 * Copyright 2016 by Valentin Rothberg <valentinrothberg@gmail.com>
 * Licensed under the GPL v2
 *
 *
 * This GCC plugin adds support for multiversing functions, as described by
 * [1,2].  Currently, the multiverse support is limited to 'boolean' variables
 * that must be attributed with 'multiverse'.
 *
 *
 * Further reading for function/compiler multiverse:
 *
 * [1] Kernel Newbies wiki
 *     https://kernelnewbies.org/KernelProjects/compiler-multiverse
 *
 * [2] Function Multiverses for Dynamic Variability
 *     https://www4.cs.fau.de/Publications/2016/rothberg_16_dspl.pdf
 */

#include <bitset>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <vector>

#include <string>
#include <algorithm>
#include <assert.h>

#include "gcc-common.h"
#include "mv-info.h"
#include "multiverse.h"


int plugin_is_GPL_compatible;

struct plugin_info mv_plugin_info = { .version = "42" };

static mv_info_ctx_t mv_info_ctx;

#define dump_file stderr

#define debug_printf(args...) do {               \
        if (dump_file) {                        \
            fprintf(dump_file, args);           \
        }                                       \
    } while(0)


/*
 * Handler for multiverse attribute of variables. We collect here all
 * variables that are defined in this compilation unit.
 */
static tree handle_mv_attribute(tree *node, tree name, tree args, int flags,
                                bool *no_add_attrs)
{
    int type = TREE_CODE(TREE_TYPE(*node));
    // FIXME: Error on weak attributed variables?
    if (type == INTEGER_TYPE || type == ENUMERAL_TYPE) {
        // We encountered a enumeral/int type. Therefore it should be a variable.
        if (!DECL_EXTERNAL(*node)) { // Defined in this compilation unit.
            mv_info_var_data mv_variable;
            mv_variable.var_decl = *node;
            mv_info_ctx.variables.push_back(mv_variable);
        }
    } else if (type == FUNCTION_TYPE) {
        // A function was declared as a multiversed function.
        // Everything is fine. We mark the function as unclonable to
        // avoid all kind of nasty bugs. (function splitting)
        // Furthermore, we mark the function explicitly as
        // uninlinable.
        DECL_ATTRIBUTES(*node) = tree_cons(get_identifier("noclone"), NULL,
                                           tree_cons(get_identifier("noinline"), NULL,
                                                     DECL_ATTRIBUTES(*node)));
        DECL_UNINLINABLE(*node) = 1;
    } else {
        error("variable %qD with %qE attribute must be an integer "
              "or enumeral type", *node, name);
    }

    return NULL_TREE;
}



/*
 * The struct below is required to register the __attribute__((multiverse))
 * attribute.  Please refer to tree-core.h for detailed information about this
 * struct and its members.
 */
static struct attribute_spec mv_attribute =
{
    .name = "multiverse",
    .min_length = 0,
    .max_length = 0,
    .decl_required = true,
    .type_required = false,
    .function_type_required = false,
    .handler = handle_mv_attribute,
    .affects_type_identity = false,
};




/*
 * The callback function to register the multiverse attribute.
 */
static void register_mv_attribute(void *event_data, void *data)
{
    register_attribute(&mv_attribute);
}


/*
 * Return the call graph node of fndecl.
 */
struct cgraph_node *get_fn_cnode(const_tree fndecl)
{
    gcc_assert(TREE_CODE(fndecl) == FUNCTION_DECL);
#if BUILDING_GCC_VERSION <= 4005
    return cgraph_get_node(CONST_CAST_TREE(fndecl));
#else
    return cgraph_get_node(fndecl);
#endif
}


/*
 * Return true if var is multiverse attributed.
 */
static bool is_multiverse_var(tree &var)
{
    if (!is_global_var(var) || TREE_CODE(var) != VAR_DECL)
        return false;

    tree attr = lookup_attribute("multiverse", DECL_ATTRIBUTES(var));
    if(attr == NULL_TREE)
        return false;
    return true;
}

/*
 * Return true if function is multiverse attributed.
 */
static bool is_multiverse_fn(tree &fn)
{
    if (TREE_CODE(fn) != FUNCTION_DECL)
        return false;

    tree attr = lookup_attribute("multiverse", DECL_ATTRIBUTES(fn));
    if(attr == NULL_TREE)
        return false;
    return true;
}


/*
 * Build a clone of FNDECL with a modified name.
 *
 * Code bases on chkp_maybe_create_clone().
 */
static tree clone_fndecl(tree fndecl, std::string fname)
{
    tree new_decl;
    cgraph_node * node;
    cgraph_node * clone;

    new_decl = copy_node(fndecl);

    DECL_NAME(new_decl) = get_identifier(fname.c_str());

    SET_DECL_ASSEMBLER_NAME(new_decl, get_identifier(fname.c_str()));

    // Not sure if we need to copy the attributes list: better safe than sorry
    DECL_ATTRIBUTES(new_decl) = copy_list(DECL_ATTRIBUTES(fndecl));

    DECL_FUNCTION_CODE(new_decl);

    node = get_fn_cnode(fndecl);
    clone = node->create_version_clone(new_decl, vNULL, NULL);
    clone->externally_visible = true;   // just to avoid being removed
    clone->local = node->local;
    clone->address_taken = node->address_taken;
    clone->thunk = node->thunk;
    clone->alias = node->alias;
    clone->weakref = node->weakref;
    clone->cpp_implicit_alias = node->cpp_implicit_alias;

    if (gimple_has_body_p(fndecl)) {
        tree_function_versioning(fndecl, new_decl, NULL, NULL,
                                 NULL, false, NULL, NULL);
        clone->lowered = true;
    }

    return new_decl;
}


/*
 * Replace all occurrences of var in cfun with value.
 */
static void replace_and_constify(tree old_var, const int value)
{
    tree new_var = build_int_cst(TREE_TYPE(old_var), value);

    basic_block bb;
    FOR_EACH_BB_FN(bb, cfun) {
        // iterate over each GIMPLE statement
        gimple_stmt_iterator gsi;
        for (gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
            gimple stmt = gsi_stmt(gsi);
            if (!is_gimple_assign(stmt)) {
                continue;
            }

            // check if any operand is a multiverse variable
            for (int num = 1; num < gimple_num_ops(stmt); num++) {
                tree var = gimple_op(stmt, num);
                if (var != old_var)
                    continue;

                gimple_set_op(stmt, num, new_var);
                update_stmt(stmt);

                if (dump_file) {
                    fprintf(dump_file, ".. found multiverse variable in statement, replacing: ");
                    print_gimple_stmt(dump_file, stmt, 0, TDF_SLIM);
                }
            }
        }
    }
}


/*
 * Clone the current function and replace all variables in @assignment with the
 * associated values.
 */
static void multiverse_function(mv_info_fn_data &fn_info,
                                mv_variant_generator::variant assignment)
{
    tree fndecl = cfun->decl;
    tree clone;
    function * old_func = cfun;
    function * clone_func;

    std::string fname = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(fndecl));
    std::stringstream ss;

    for (mv_variant_generator::dimension_value &val : assignment) {
        // Append variable assignments to the clone's name
        ss << "_" << IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(val.variable));
        if (val.value_label)
            ss << val.value_label;
        else
            ss << val.value;
    }

    fname += ".multiverse.";
    fname += ss.str().substr(1);

    if (dump_file) {
        fprintf(dump_file, "generating function clone %s\n",
                fname.c_str(), IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(fndecl)));
    }

    clone = clone_fndecl(fndecl, fname);
    gcc_assert(clone != fndecl);

    clone_func = DECL_STRUCT_FUNCTION(clone);
    push_cfun(clone_func);
    mv_info_mvfn_data mvfn;
    mvfn.mvfn_decl = clone; // Store declartion for clone

    for (mv_variant_generator::dimension_value &val : assignment) {
        replace_and_constify(val.variable, val.value);

        // Save the information about the assignment
        mv_info_assignment_data assign;
        assign.var_name = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(val.variable));
        assign.var_decl = val.variable;
        assign.lower_limit = val.value;
        assign.upper_limit = val.value;
        mvfn.assignments.push_back(assign);
    }

    fn_info.mv_functions.push_back(mvfn);
    remove_attribute("multiverse", DECL_ATTRIBUTES(cfun->decl));
    pop_cfun();

    return;
}


void mv_variant_generator::add_dimension(tree variable, unsigned score) {
    bool found = false;
    for (const auto &item : dimensions) {
        found |= (item.variable == variable);
    }
    assert(!found && "Dimension added multiple times");
    dimensions.push_back({variable, score});
}

void mv_variant_generator::add_dimension_value(tree variable,
                                               const char *label,
                                               unsigned HOST_WIDE_INT value,
                                               unsigned score) {
    bool found = false;
    for (auto &dim : dimensions) {
        if (dim.variable == variable) {
            found = true;
            dim.values.push_back({variable, label, value, score});
            dim.score += score;
        }
    }
    assert(found && "Dimension not found");
}

void mv_variant_generator::start(unsigned maximal_elements) {
    // First, we sort dimensions and dimension values according to
    // their score. The highest scores are sorted to the front
    state.clear();
    for (auto &dim : dimensions) {
        std::sort(dim.values.begin(), dim.values.end(),
                  [](dimension_value &a, dimension_value &b) {
                      return b.score > a.score;
                  });
        state.push_back(0);
    }
    std::sort(dimensions.begin(), dimensions.end(),
              [](dimension &a, dimension &b) {
                  return b.score > a.score;
              });
    // We calculate how many "unimportant" dimensions are skipped,
    // since we never will variante in their assignment, because
    // we never get there (maximal_elements)
    if (maximal_elements == -1) {
        skip_dimensions = 0;
    } else {
        unsigned long long strength = 1;
        for (unsigned i = dimensions.size() - 1; i >= 0; --i) {
            strength *= dimensions[i].values.size();
            if (strength >= maximal_elements ) {
                skip_dimensions = i;
                break;
            }
        }
    }
    element_count = 0;
    this->maximal_elements = maximal_elements;
}

bool mv_variant_generator::end_p() {
    return ((element_count >= maximal_elements)
            || state[0] >= dimensions[0].values.size());
}

mv_variant_generator::variant mv_variant_generator::next() {
    variant ret;
    if (end_p()) return ret; // Generator ended

    // Capture current dimensions
    for (unsigned i = skip_dimensions; i < dimensions.size(); i++) {
        ret.push_back(dimensions[i].values[state[i]]);
    }
    // and incrmeent to the next element
    state[dimensions.size() -1] ++;
    // Overflow for all dimensions but first one i != 0
    for (unsigned i = dimensions.size() - 1; i > 0; --i) {
        if (state[i] >= dimensions[i].values.size()) {
            state[i] = 0;
            state[i-1] ++;
        }
    }

    element_count++;
    return ret;
}

/*
 * Pass to find multiverse attributed variables in the current function.  In
 * case such variables are used in conditional statements, the functions is
 * cloned and specialized (constant propagation, etc.).
 */
static unsigned int mv_variant_generation_execute()
{
    std::string fname = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(cfun->decl));

    bool is_attributed_mv = is_multiverse_fn(cfun->decl);

    std::set<tree> mv_vars;
    std::map<tree, std::set<unsigned HOST_WIDE_INT>> mv_var_hints;

    std::set<tree> mv_blacklist;
    basic_block bb;
    /* Iterate of each basic block in current function. */
    FOR_EACH_BB_FN(bb, cfun) {
        /* Iterate over each GIMPLE statement. */
        gimple_stmt_iterator gsi;
        gimple last_stmt = NULL;
        gimple stmt = NULL;
        for (gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
            last_stmt = stmt;
            stmt = gsi_stmt(gsi);

            if (!is_gimple_assign(stmt)) {
                continue;
            }

            /*
             * If left hand side is a multiverse var, then don't use it for
             * multiversing the current function.
             */
            tree lhs = gimple_assign_lhs(stmt);
            if (is_multiverse_var(lhs)) {
                if (dump_file) {
                    fprintf(dump_file, "function assigns to multiverse variable: %s; not specializing\n",
                            IDENTIFIER_POINTER(DECL_NAME(lhs)));
                }
                mv_blacklist.insert(lhs);
                continue;
            }

            /* Check if any operand is a multiverse variable */
            for (int num = 1; num < gimple_num_ops(stmt); num++) {
                tree var = gimple_op(stmt, num);

                if (!is_multiverse_var(var))
                    continue;

                // Function might not be attributed with multiverse. Emit a warning
                if (!is_attributed_mv) {
                    location_t loc = gimple_location(stmt);
                    warning_at(loc, OPT_Wextra, "function uses multiverse variables without being multiverse itself");
                    continue;
                }
                if (dump_file) {
                    fprintf(dump_file, "found multiverse operand: ");
                    print_generic_stmt(dump_file, var, 0);
                }
                if (gimple_num_ops(stmt) == 2 && TREE_CODE(TREE_TYPE(var)) == INTEGER_TYPE) {
                    // We can try to guess the value
                    fprintf(stderr, "guess!");
                    tree local_var = gimple_op(stmt, 0);
                    gimple use_stmt;
                    imm_use_iterator imm_iter;
                    FOR_EACH_IMM_USE_STMT(use_stmt, imm_iter, local_var) {
                        if (gimple_code(use_stmt) == GIMPLE_COND
                            && gimple_cond_code(use_stmt) == EQ_EXPR) {
                            tree comparand;
                            if (gimple_op(use_stmt, 0) == local_var)
                                comparand = gimple_op(use_stmt, 1);
                            else if (gimple_op(use_stmt, 1) == local_var)
                                comparand = gimple_op(use_stmt, 0);
                            else
                                assert(false && "Could not find variable in comparision");

                            if (!CONSTANT_CLASS_P(comparand)) continue;

                            unsigned HOST_WIDE_INT constant = int_cst_value(comparand);
                            if (mv_var_hints.find(var) == mv_var_hints.end()) {
                                mv_var_hints[var] = {constant};
                            } else {
                                mv_var_hints[var].insert(constant);
                            }
                        }
                    }
                }
                mv_vars.insert(var);
            }
        }
    }

    if (dump_file) {
        fprintf(dump_file, "...found '%d' multiverse variables (%d blacklisted)\n",
                mv_vars.size(), mv_blacklist.size());
    }
    std::set<tree>::iterator varit;
    for (varit = mv_blacklist.begin(); varit != mv_blacklist.end(); varit++) {
        mv_vars.erase(*varit);
    }

    /* Multiverse this function w.r.t. multiverse variables */
    const int numvars = mv_vars.size();
    if (numvars == 0)
        return 0;

    /* Here, we know that we will generate multiverse variants. We use
     * generators to generate all of them */
    mv_variant_generator generator;
    for (auto & variable : mv_vars) {
        if (TREE_CODE(TREE_TYPE(variable)) == ENUMERAL_TYPE) {
            generator.add_dimension(variable);

            tree element;
            for (element = TYPE_VALUES (TREE_TYPE (variable));
                 element != NULL_TREE;
                 element = TREE_CHAIN (element)) {
                const char * label = IDENTIFIER_POINTER(TREE_PURPOSE(element));
                if (tree_fits_uhwi_p (TREE_VALUE (element))) {
                    unsigned HOST_WIDE_INT val = tree_to_uhwi (TREE_VALUE (element));
                    generator.add_dimension_value(variable, label, val, 1);
                }
            }
        } else if (TREE_CODE(TREE_TYPE(variable)) == INTEGER_TYPE) {
            generator.add_dimension(variable);
            if (mv_var_hints.find(variable) != mv_var_hints.end()) {
                for (auto val : mv_var_hints[variable]) {
                    generator.add_dimension_value(variable, NULL, val, 0);
                }
            } else {
                generator.add_dimension_value(variable, NULL, 0, 0);
                generator.add_dimension_value(variable, NULL, 1, 0);
            }
        }
    }

    // The Generator is filled with data about our variables. Let's
    // generate all variants for this function
    mv_info_ctx.functions.push_back(mv_info_fn_data());
    mv_info_fn_data &fn_data = mv_info_ctx.functions.back();
    fn_data.fn_decl = cfun->decl;

    generator.start();
    while (!generator.end_p()) {
        std::vector<mv_variant_generator::dimension_value> assignment;
        assignment = generator.next();
        multiverse_function(fn_data, assignment);
    }

    return 0;
}


#define PASS_NAME mv_variant_generation
#define NO_GATE
#include "gcc-generate-gimple-pass.h"

typedef std::vector<
    std::pair<mv_info_mvfn_data*,
              ipa_icf::sem_function *>> equivalence_class;

// For two assignment maps, we test if they differ only in one
// variable assignment. If they differ, we furthermore test, wheter
// their intervals are next to each other. In this case, we merge b
// into a, and say: yes, we merged something here.
static bool
merge_if_possible(std::vector<mv_info_assignment_data> &a,
                     std::vector<mv_info_assignment_data> &b) {
    if (a.size() != b.size()) return false;

    unsigned differences = 0, idx = 0;
    bool compatible = false;
    unsigned lower, upper;
    for (unsigned i = 0; i < a.size() ; i++) {
        assert(a[i].var_decl == b[i].var_decl);
        if (a[i].lower_limit == b[i].lower_limit
            && a[i].upper_limit == b[i].upper_limit)
            continue;
        differences ++;
        if (a[i].lower_limit == (b[i].upper_limit-1)
            || b[i].lower_limit == (a[i].upper_limit +1)) {
            idx = i;
            lower = std::min(a[i].lower_limit, b[i].lower_limit);
            upper = std::max(a[i].upper_limit, b[i].upper_limit);
            compatible = true;
        }
    }
    if (differences == 1 && compatible) {
        a[idx].lower_limit = lower;
        a[idx].upper_limit = upper;
        return true;
    }
    return false;
}

/* In a mvfn selector equivalence class, all selectors point to the
   same mvfn function body. Nevertheless, their guarding assignment
   maps may be different. In some cases, we can reduce the number of
   descriptors, by merging their intervals.

   WARNING: I think this merging is not optimal */
static
int merge_mvfn_selectors(struct mv_info_fn_data &fn_info,
                         equivalence_class &ec) {
    debug_printf("\nmerge mvfn descriptors for: %s\n",
                 IDENTIFIER_POINTER(DECL_NAME(fn_info.fn_decl)));

    bool changed = true;
    while (changed) {
        changed = false;
        // We want to merge b into a if possible
        for (unsigned a = 0; a < ec.size(); a++) {
            for (unsigned b = a+1; b < ec.size(); b++) {
                if (dump_file) {
                    ec[a].first->dump(dump_file);
                    debug_printf(" <-> ");
                    ec[b].first->dump(dump_file);
                    debug_printf("\n");
                }
                // Merge B in A
                bool merged = merge_if_possible(ec[a].first->assignments,
                                                ec[b].first->assignments);
                unsigned dst = a, remove = b;
                if (!merged) {
                    // Did not work? Merge A into B
                    merged = merge_if_possible(ec[b].first->assignments,
                                               ec[a].first->assignments);
                    remove = a; dst = b;
                }
                if (merged) {
                    if (dump_file) {
                        debug_printf(" ->>  Merged: ");
                        ec[dst].first->dump(dump_file);
                        debug_printf("\n");
                    }
                    changed = true;
                    // Remove from the global list of mvfn_function descriptors
                    for (auto it = fn_info.mv_functions.begin();
                         it != fn_info.mv_functions.end(); it++) {
                        if (&*it == ec[remove].first) {
                            fn_info.mv_functions.erase(it);
                            break;
                        }
                    }
                    // Remove second mvfn descriptor
                    delete ec[remove].second; // sem_function *
                    ec.erase(ec.begin() + remove);
                }
            }
        }
    }
    return 0;
}

/*
 */
static unsigned int mv_variant_elimination_execute() {
    using namespace ipa_icf;

    bitmap_obstack bmstack;
    bitmap_obstack_initialize(&bmstack);

    for (auto &fn_info : mv_info_ctx.functions) {
        debug_printf("\nmerge function bodies for: %s\n",
                     IDENTIFIER_POINTER(DECL_NAME(fn_info.fn_decl)));

        std::list<equivalence_class> classes;
        hash_map <symtab_node *, sem_item *> ignored_nodes;
        for (auto &mvfn_info : fn_info.mv_functions) {
            cgraph_node *node = get_fn_cnode(mvfn_info.mvfn_decl);
            sem_function *func = new sem_function(node, 0, &bmstack);
            func->init();

            debug_printf("%s ", IDENTIFIER_POINTER(DECL_NAME(mvfn_info.mvfn_decl)));

            bool found = false;
            for (auto &ec : classes) {
                bool eq = ec.front().second->equals(func, ignored_nodes);
                if (eq) {
                    debug_printf(" EQ; add to existing equivalence class\n");
                    ec.push_back({&mvfn_info, func});
                    found = true;
                    break;
                }
            }
            // None found? Start a new one!
            if (!found) {
                debug_printf("NEQ; new equivalence class\n");
                classes.push_back({{&mvfn_info,func}});
            }
        }
        for (auto &ec : classes) {
            debug_printf("found function equivalence class of size %d\n", ec.size());
            /* If multiple multiverse functions are equivalent. Let
               them all point to the same function body. */
            if (ec.size() > 1) {
                // The first in the equivalence class is our representant
                auto &first = ec[0];
                for (unsigned i = 1; i < ec.size(); i++) {
                    auto &other = ec[i];
                    // Mark all others as disposable
                    cgraph_node *other_node = get_fn_cnode(other.second->decl);
                    other_node->externally_visible = false;
                    other_node->make_local();

                    // We reference the one variant that is not removed.
                    other.first->mvfn_decl = first.first->mvfn_decl;
                }
                // Merge equivalence classes of selektors
                // Think of the following situation:
                // ec = [(a=0, b=0), (a=0, b=1), (a=1, b=0)]
                // Then it would be sufficient to expose the following terms
                merge_mvfn_selectors(fn_info, ec);
                debug_printf(".. reduced to size %d\n", ec.size());
            }
        }
        // Cleanup of sem_function *
        for (const auto &ec : classes) {
            for (const auto &item : ec) {
                delete item.second;
            }
        }
    }

    bitmap_obstack_release(&bmstack);

    return TODO_remove_functions;
}

#define PASS_NAME mv_variant_elimination
#define NO_GATE
#include "gcc-generate-simple_ipa-pass.h"

/*
 * Pass to find call instructions in the RTL that references a
 * multiverse function. For such callsites, we insert a label and
 * record it for the mv_info.
 */
static unsigned int mv_callsites_execute()
{
    std::string fname = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(cfun->decl));
    // debug_print("rtl: %p %s\n", cfun, fname.c_str());
    basic_block b;
    rtx_insn *insn;
    FOR_EACH_BB_FN (b, cfun) {
        FOR_BB_INSNS (b, insn) {
            rtx call;
            if (CALL_P(insn) && (call = get_call_rtx_from(insn))) {
                tree decl = SYMBOL_REF_DECL (XEXP(XEXP(call,0), 0));
                if (!decl) continue;
                if (!is_multiverse_fn(decl)) continue;

                // We have to insert a label before the code
                tree tree_label = create_artificial_label (UNKNOWN_LOCATION);
                rtx label = jump_target_rtx (tree_label);
                LABEL_NUSES(label) = 1;
                emit_label_before (label, insn);

                // Sonce jump_target_rtx emits a code_label, which is
                // not allowed within a basic block, we transform it
                // to a NOTE_INSN_DELETEC_LABEL
                PUT_CODE (label, NOTE);
                NOTE_KIND (label) = NOTE_INSN_DELETED_LABEL;

                mv_info_callsite_data callsite;
                callsite.fn_decl = decl;
                callsite.callsite_label = tree_label;
                mv_info_ctx.callsites.push_back(callsite);
            }
        }
    }

    return 0;
}


#define PASS_NAME mv_callsites
#define NO_GATE
#include "gcc-generate-rtl-pass.h"


/*
 * Initialization function of this plugin: the very heart & soul.
 */
int plugin_init(struct plugin_name_args *info, struct plugin_gcc_version *version)
{
    const char * plugin_name = info->base_name;
    struct register_pass_info mv_variant_generation_info;
    struct register_pass_info mv_variant_elimination_info;
    struct register_pass_info mv_callsites_info;

    if (!plugin_default_version_check(version, &gcc_version)) {
        error(G_("incompatible gcc/plugin versions"));
        return 1;
    }

    // Initialize types and the multiverse info structures.
    register_callback(plugin_name, PLUGIN_START_UNIT, mv_info_init, &mv_info_ctx);

    // register plugin information
    register_callback(plugin_name, PLUGIN_INFO, NULL, &mv_plugin_info);

    // register the multiverse attribute
    register_callback(plugin_name, PLUGIN_ATTRIBUTES, register_mv_attribute, NULL);

    // register pass: generate multiverse variants
    mv_variant_generation_info.pass = make_mv_variant_generation_pass();
    mv_variant_generation_info.reference_pass_name = "ssa";
    mv_variant_generation_info.ref_pass_instance_number = 0;
    mv_variant_generation_info.pos_op = PASS_POS_INSERT_AFTER; // AFTER => more optimized code
    register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &mv_variant_generation_info);

    // register pass: eliminate duplicated variants
    mv_variant_elimination_info.pass = make_mv_variant_elimination_pass();
    mv_variant_elimination_info.reference_pass_name = "opt_local_passes";
    mv_variant_elimination_info.ref_pass_instance_number = 0;
    mv_variant_elimination_info.pos_op = PASS_POS_INSERT_AFTER; // AFTER => more optimized code
    register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &mv_variant_elimination_info);

    // register the multiverse RTL pass which adds labels to callsites
    mv_callsites_info.pass = make_mv_callsites_pass();
    mv_callsites_info.reference_pass_name = "final";
    mv_callsites_info.ref_pass_instance_number = 0;
    mv_callsites_info.pos_op = PASS_POS_INSERT_BEFORE; // BEFORE => no assembly yet
    register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &mv_callsites_info);

    // Finish off the generation of multiverse info
    register_callback(plugin_name, PLUGIN_FINISH_UNIT, mv_info_finish, &mv_info_ctx);

    return 0;
}
