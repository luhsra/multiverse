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
#include <string>
#include <algorithm>
#include <assert.h>

#include "gcc-common.h"
#include "mv-info.h"
#include "multiverse.h"



#define MV_SUFFIX ".multiverse"


int plugin_is_GPL_compatible;

struct plugin_info mv_plugin_info = { .version = "42" };

static mv_info_ctx_t mv_info_ctx;

static unsigned int find_mv_vars_execute();


/*
 * Handler for multiverse attribute of variables. We collect here all
 * variables that are defined in this compilation unit.
 */
static tree handle_mv_attribute(tree *node, tree name, tree args, int flags,
                                bool *no_add_attrs)
{
#ifdef DEBUG
    fprintf(stderr, "---- attributing MULTIVERSE variable ");
    fprintf(stderr, "(extern = %d) ", DECL_EXTERNAL(*node));
    print_generic_stmt(stderr, *node, 0);
#endif

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
        // Everything is fine.
    } else {
        // FIXME: Error message for functions
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

#ifdef DEBUG
            fprintf(stderr, "---- Checking operands in statement: ");
            print_gimple_stmt(stderr, stmt, 0, TDF_SLIM);
#endif

            if (!is_gimple_assign(stmt)) {
#ifdef DEBUG
                fprintf(stderr, "...skipping non-assign statement\n");
#endif
                continue;
            }

            // check if any operand is a multiverse variable
            for (int num = 1; num < gimple_num_ops(stmt); num++) {
                tree var = gimple_op(stmt, num);
                if (var != old_var)
                    continue;

                gimple_set_op(stmt, num, new_var);
                update_stmt(stmt);
                fprintf(stderr, "...replacing operand in this statement: ");
                print_gimple_stmt(stderr, stmt, 0, TDF_SLIM);
            }
        }
    }
}



/*
 * Clone the current function and replace all variables in @var_map with the
 * associated values.
 */
static void multiverse_function(mv_info_fn_data &fn_info, std::map<tree, int> var_map)
{
    tree fndecl = cfun->decl;
    tree clone;
    function * old_func = cfun;
    function * clone_func;

    std::map<tree, int>::iterator map_iter;
    std::string fname = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(fndecl));
    for (map_iter = var_map.begin(); map_iter != var_map.end(); map_iter++) {
        tree var = map_iter->first;
        int val = map_iter->second;

        // Append variable assignments to the clone's name
        std::stringstream ss;
        ss << "_" << IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(var)) << val;
        fname += ss.str();
    }
    fname += MV_SUFFIX;


#ifdef DEBUG
    fprintf(stderr, "---- Generating function clone '%s'\n", fname.c_str());
#endif

    clone = clone_fndecl(fndecl, fname);
    gcc_assert(clone != fndecl);

    // WTF is this doing.
    clone_func = DECL_STRUCT_FUNCTION(clone);
    push_cfun(clone_func);
    mv_info_mvfn_data mvfn;
    mvfn.mvfn_decl = clone; // Store declartion for clone
    for (map_iter = var_map.begin(); map_iter != var_map.end(); map_iter++) {
        tree var = map_iter->first;
        int val = map_iter->second;
        replace_and_constify(var, val);

        // Save the information about the assignment
        mv_info_assignment_data assign;
        assign.var_decl = var;
        assign.lower_limit = val;
        assign.upper_limit = val;
        mvfn.assignments.push_back(assign);
    }
    fn_info.mv_functions.push_back(mvfn);


    DECL_ATTRIBUTES(cfun->decl) =
        remove_attribute("multiverse", DECL_ATTRIBUTES(cfun->decl));

    pop_cfun();

    return;
}


/*
 * Return true if fndecl is cloneable (i.e., not main).
 */
bool is_cloneable_function(tree fndecl)
{
    std::string fname = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(fndecl));

    if (fname.compare("main") == 0)
        return false;

    return true;
}


/*
 * Pass to find multiverse attributed variables in the current function.  In
 * case such variables are used in conditional statements, the functions is
 * cloned and specialized (constant propagation, etc.).
 */
static unsigned int find_mv_vars_execute()
{
    std::string fname = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(cfun->decl));
#ifdef DEBUG
    fprintf(stderr, "************************************************************\n");
    print_current_pass(stderr);
    fprintf(stderr, "**** Searching multiverse variables in '%s'\n\n", fname.c_str());
#endif

/*
    if (!is_cloneable_function(cfun->decl)) {
#ifdef DEBUG
        fprintf(stderr, ".... skipping, it's not multiverseable\n");
        fprintf(stderr, "************************************************************\n\n");
#endif
        return 0;
    }
*/

    bool is_attributed_mv = is_multiverse_fn(cfun->decl);

    std::set<tree> mv_vars;
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
#ifdef DEBUG
            fprintf(stderr, "---- Checking operands in statement: ");
            // print_gimple_stmt(stderr, stmt, 0, TDF_SLIM);
#endif

            if (!is_gimple_assign(stmt)) {
                debug_print("...skipping non-assign statement\n");
                continue;
            }

            /*
             * If left hand side is a multiverse var, then don't use it for
             * multiversing the current function.
             */
            tree lhs = gimple_assign_lhs(stmt);
            if (is_multiverse_var(lhs)) {
#ifdef DEBUG
                fprintf(stderr, "...skipping function: assign to multiverse variable\n");
#endif
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
                    warning_at(loc, 0, "function uses multiverse variables without being multiverse itself");
                    continue;
                }
#ifdef DEBUG
                fprintf(stderr, "...found multiverse operand: ");
                print_generic_stmt(stderr, var, 0);
#endif
                mv_vars.insert(var);


            }
        }


    }

#ifdef DEBUG
    fprintf(stderr, "...found '%d' multiverse variables\n", mv_vars.size());
#endif
    std::set<tree>::iterator varit;
    for (varit = mv_blacklist.begin(); varit != mv_blacklist.end(); varit++) {
#ifdef DEBUG
        fprintf(stderr, "...removing blacklisted variable: ");
        print_generic_stmt(stderr, *varit, 0);
#endif
        mv_vars.erase(*varit);
    }

    /* Multiverse this function w.r.t. multiverse variables */
    const int numvars = mv_vars.size();
    if (numvars == 0)
        return 0;

    std::map<tree, int> var_map;
    tree var_list[numvars];
    std::copy(mv_vars.begin(), mv_vars.end(), var_list);

    for (int i = 0; i < numvars; i++)
        var_map[var_list[i]] = 0;

    mv_info_ctx.functions.push_back(mv_info_fn_data());
    mv_info_fn_data &fn_data = mv_info_ctx.functions.back();
    fn_data.fn_decl = cfun->decl;

    for (int i = 0; i < (1 << numvars); i++) {
        std::bitset <sizeof(int)> bits(i);
        std::map<tree, int> var_map_i(var_map);

        // new assignment for current bitset
        for (int j = 0; j < numvars; j++) {
            var_map_i[var_list[j]] = bits[j];
        }
        multiverse_function(fn_data,var_map_i);
    }

#ifdef DEBUG
    fprintf(stderr, "************************************************************\n\n");
#endif
    return 0;
}


#define PASS_NAME find_mv_vars
#define NO_GATE
#include "gcc-generate-gimple-pass.h"

/*
 */
static unsigned int mv_variant_elimination_execute() {
    using namespace ipa_icf;

    debug_print("IPA\n");

    bitmap_obstack bmstack;
    bitmap_obstack_initialize(&bmstack);

    for (auto &fn_info : mv_info_ctx.functions) {
        std::list<std::list<std::pair<mv_info_mvfn_data*, sem_function *>>> classes;
        hash_map <symtab_node *, sem_item *> ignored_nodes;
        for (auto &mvfn_info : fn_info.mv_functions) {
            cgraph_node *node = get_fn_cnode(mvfn_info.mvfn_decl);
            sem_function *func = new sem_function(node, 0, &bmstack);
            func->init();

            std::string fname = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(mvfn_info.mvfn_decl));
            debug_print("%s ", fname.c_str());

            bool found = false;
            for (auto &ec : classes) {
                bool eq = ec.front().second->equals(func, ignored_nodes);
                if (eq) {
                    debug_print("EQ\n");
                    ec.push_back({&mvfn_info, func});
                    found = true;
                    break;
                }
            }
            // None found? Start a new one!
            if (!found) {
                debug_print("NEQ\n");
                classes.push_back({{&mvfn_info,func}});
            }
        }
        for (auto &ec : classes) {
            /* If multiple multiverse functions are equivalent. Let
               them all point to the same function body. */
            if (ec.size() > 1) {
                auto first = ec.front();
                ec.pop_front();
                for (const auto &other: ec) {
                    // Mark all others as disposable
                    cgraph_node *other_node = get_fn_cnode(other.second->decl);
                    other_node->externally_visible = false;
                    other_node->make_local();

                    // We reference the one variant that is not removed.
                    other.first->mvfn_decl = first.first->mvfn_decl;
                }
                ec.push_front(first);
            }
            debug_print("found equivalence class of size %d\n", ec.size());
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
static unsigned int find_mv_callsites_execute()
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

                // debug_print("mv-call: %p\n", call);
                // print_rtl(stderr, call);
                // print_generic_stmt(stderr, decl, 0);

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


#define PASS_NAME find_mv_callsites
#define NO_GATE
#include "gcc-generate-rtl-pass.h"


/*
 * Initialization function of this plugin: the very heart & soul.
 */
int plugin_init(struct plugin_name_args *info, struct plugin_gcc_version *version)
{
#ifdef DEBUG
    fprintf(stderr, "---- Initializing the multiverse GCC plugin.\n");
#endif

    const char * plugin_name = info->base_name;
    struct register_pass_info find_mv_vars_info;
    struct register_pass_info find_mv_callsites_info;
    struct register_pass_info mv_variant_elimination_info;


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
    find_mv_vars_info.pass = make_find_mv_vars_pass();
    find_mv_vars_info.reference_pass_name = "ssa";
    find_mv_vars_info.ref_pass_instance_number = 0;
    find_mv_vars_info.pos_op = PASS_POS_INSERT_AFTER; // AFTER => more optimized code
    register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &find_mv_vars_info);

    // register pass: eliminate duplicated variants
    mv_variant_elimination_info.pass = make_mv_variant_elimination_pass();
    mv_variant_elimination_info.reference_pass_name = "opt_local_passes";
    mv_variant_elimination_info.ref_pass_instance_number = 0;
    mv_variant_elimination_info.pos_op = PASS_POS_INSERT_AFTER; // AFTER => more optimized code
    register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &mv_variant_elimination_info);

    // register the multiverse RTL pass
    find_mv_callsites_info.pass = make_find_mv_callsites_pass();
    find_mv_callsites_info.reference_pass_name = "final";
    find_mv_callsites_info.ref_pass_instance_number = 0;
    find_mv_callsites_info.pos_op = PASS_POS_INSERT_BEFORE; // BEFORE => no assembly yet
    register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &find_mv_callsites_info);

    // Finish off the generation of multiverse info
    register_callback(plugin_name, PLUGIN_FINISH_UNIT, mv_info_finish, &mv_info_ctx);

    return 0;
}
