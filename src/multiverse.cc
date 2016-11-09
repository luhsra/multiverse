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

#include "gcc-common.h"

#include "tree-iterator.h"

#include <bitset>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>

#define MV_SUFFIX ".multiverse"
#define MV_VERSION 42


int plugin_is_GPL_compatible;


struct plugin_info mv_plugin_info = { .version = "42" };


struct mv_info_assignment_data {
    tree var_decl;
    unsigned lower_limit;
    unsigned upper_limit;
};


struct mv_info_fn_data;


struct mv_info_mvfn_data {
    tree mvfn_decl;
    std::list<mv_info_assignment_data> assignments;
    mv_info_fn_data *parent_fn_data;
};


struct mv_info_fn_data {
    tree fn_decl;			 /* the function decl */
    std::list<mv_info_mvfn_data> mv_functions;
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

    // information
    std::list<mv_info_var_data> variables;
    std::list<mv_info_fn_data> functions;
} mv_info_ctx_t;


static mv_info_ctx_t mv_info_ctx;

static unsigned int find_mv_vars_execute();
static tree build_info_mvfn(mv_info_mvfn_data &, mv_info_ctx_t *ctx);
static tree build_info_assignment(mv_info_assignment_data &, mv_info_ctx_t *ctx);


#ifdef DEBUG
#define debug_print(args...) fprintf(stderr, args)
#else
#define debug_print(args...) do {} while(0)
#endif


/*
 * Handler for multiverse attributing.  Currently it's only used for debugging
 * purposes but maybe we need to do something at a later point.
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
    if (type != INTEGER_TYPE && type != ENUMERAL_TYPE)
        error("variable %qD with %qE attribute must be an integer "
              "or enumeral type", *node, name);


    // FIXME: Error on weak attributed variables?

    if (!DECL_EXTERNAL(*node)) { // Defined in this compilation unit.
        mv_info_var_data mv_variable;
        mv_variable.var_decl = *node;
        mv_info_ctx.variables.push_back(mv_variable);
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
    .decl_required = false,
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


static tree get_mv_unsigned_t(void)
{
    machine_mode mode = smallest_mode_for_size(32, MODE_INT);
    return lang_hooks.types.type_for_mode(mode, true);
}


/*
 * Build a coverage variable of TYPE for function FN_DECL.  If COUNTER >= 0 it
 * is a counter array, otherwise it is the function structure.
 */
static tree build_var(tree type, std::string prefix)
{
    tree var = build_decl(BUILTINS_LOCATION, VAR_DECL, NULL_TREE, type);
    static int counter = 0;
    std::stringstream ss;
    ss << prefix << counter ++;
    std::string name = ss.str();

    DECL_NAME(var) = get_identifier(name.c_str());
    TREE_STATIC(var) = 1;
    TREE_ADDRESSABLE(var) = 1;
    DECL_NONALIASED(var) = 1;
    DECL_ALIGN(var) = TYPE_ALIGN(type);

    return var;
}


template<typename T>
tree build_array_from_list(std::list<T> &elements, tree element_type,
                           tree(*build_obj_ptr)(T&, mv_info_ctx_t*),
                           mv_info_ctx_t *ctx)
{
    tree ary_type = build_array_type(build_qualified_type(element_type, TYPE_QUAL_CONST),
                                     build_index_type(size_int(elements.size())));

    vec<constructor_elt, va_gc> *ary_ctor = NULL;
    for (auto &data : elements) {
        tree obj = build_obj_ptr(data, ctx);
        CONSTRUCTOR_APPEND_ELT(ary_ctor, NULL, obj);
    }

    tree ary = build_var(ary_type, "__mv_ary_");
    DECL_INITIAL(ary) = build_constructor(ary_type, ary_ctor);
    varpool_node::finalize_decl(ary);

    return ary;
}


#define RECORD_FIELD(type) \
    field = build_decl(BUILTINS_LOCATION, FIELD_DECL, NULL_TREE, \
                       (type)); \
    DECL_CHAIN(field) = fields; \
    fields = field;


static void build_info_type(tree info_type, tree info_var_ptr_type,
                            tree info_fn_ptr_type)
{
    /*  struct __mv_info {
          unsigned int version;

          struct mv_info *next;

          unsigned int n_variables;
          struct mv_info_var *variables;

          unsigned int n_functions;
          struct mv_info_fn * functions;
        };
    */

    tree field, fields = NULL_TREE;

    /* Version ident */
    RECORD_FIELD(get_mv_unsigned_t());

    /* next pointer */
    RECORD_FIELD(build_pointer_type(build_qualified_type (info_type, TYPE_QUAL_CONST)));

    /* n_vars */
    RECORD_FIELD(get_mv_unsigned_t());

    /* struct mv_info_var pointer pointer */
    RECORD_FIELD(build_qualified_type(info_var_ptr_type, TYPE_QUAL_CONST));

    /* n_functions */
    RECORD_FIELD(get_mv_unsigned_t());

    /* struct mv_info_fn pointer pointer */
    RECORD_FIELD(build_qualified_type(info_fn_ptr_type, TYPE_QUAL_CONST));

    finish_builtin_struct(info_type, "__mv_info", fields, NULL_TREE);
}


static void build_info_fn_type(tree info_fn_type, tree info_mvfn_ptr_type)
{
    /*
      struct __mv_info_fn {
        char * const const name;
        void * original_function;

        unsigned int n_mv_functions;
        struct mv_info_mvfn ** mv_functions;
      };
    */

    tree field, fields = NULL_TREE;

    /* Name of function */
    RECORD_FIELD(build_pointer_type(build_qualified_type(char_type_node, TYPE_QUAL_CONST)));

    /* Pointer to original function body */
    RECORD_FIELD(build_pointer_type(void_type_node));

    /* n_mv_functions */
    RECORD_FIELD(get_mv_unsigned_t());

    /* mv_functions pointer */
    RECORD_FIELD(build_qualified_type(info_mvfn_ptr_type, TYPE_QUAL_CONST));

    finish_builtin_struct(info_fn_type, "__mv_info_fn", fields, NULL_TREE);
}


tree build_info_fn(mv_info_fn_data &fn_info, mv_info_ctx_t *ctx)
{
    /* Create the constructor for the top-level mv_info object */
    vec<constructor_elt, va_gc> *obj = NULL;
    tree info_fields = TYPE_FIELDS(ctx->fn_type);

    /* Name of function */
    const char *fn_name = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(fn_info.fn_decl));
    size_t fn_name_len = strlen(fn_name);
    tree fn_string = build_string(fn_name_len + 1, fn_name);

    TREE_TYPE(fn_string) = build_array_type(char_type_node,
                                            build_index_type(size_int(fn_name_len)));

    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build1(ADDR_EXPR, TREE_TYPE(info_fields), fn_string));

    info_fields = DECL_CHAIN (info_fields);

    /* Function pointer  as a (void *) */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build1(ADDR_EXPR, build_pointer_type(void_type_node),
                                  fn_info.fn_decl));

    info_fields = DECL_CHAIN(info_fields);

    /* n_mv_functions */
    unsigned n_mv_functions = fn_info.mv_functions.size();
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build_int_cstu(TREE_TYPE(info_fields), n_mv_functions));

    info_fields = DECL_CHAIN(info_fields);

    /* mv_functions -- */
    tree mvfn_ary = build_array_from_list(fn_info.mv_functions,
                                          ctx->mvfn_ptr_type,
                                          build_info_mvfn, ctx);
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build1(ADDR_EXPR, ctx->mvfn_ptr_type, mvfn_ary));
    info_fields = DECL_CHAIN(info_fields);

    gcc_assert(!info_fields); // All fields are filled

    return build_constructor(ctx->fn_type, obj);
}


static void build_info_var_type(tree info_var_type, tree info_mvfn_ptr_type)
{
    /*
      struct __mv_info_var {
        char * const name;

        void * variable;
        unsigned char width;

        int materialized_value;

        unsigned int n_mv_functions;
        struct mv_info_mvfn ** mv_functions;
      };
    */
    tree field, fields = NULL_TREE;

    /* Name of variable */
    RECORD_FIELD(build_pointer_type(build_qualified_type(char_type_node, TYPE_QUAL_CONST)));

    /* Pointer to variable */
    RECORD_FIELD(build_pointer_type(void_type_node));

    /* width */
    RECORD_FIELD(unsigned_char_type_node);

    /* materialized value */
    RECORD_FIELD(get_mv_unsigned_t());

    /* n_functions */
    RECORD_FIELD(get_mv_unsigned_t());

    /* functions pointer */
    RECORD_FIELD(build_qualified_type(info_mvfn_ptr_type, TYPE_QUAL_CONST));

    finish_builtin_struct(info_var_type, "__mv_info_var", fields, NULL_TREE);
}


static tree build_info_var(mv_info_var_data &var_info, mv_info_ctx_t *ctx)
{
    vec<constructor_elt, va_gc> *obj = NULL;
    tree info_fields = TYPE_FIELDS(ctx->var_type);

    /* name of variable */
    const char *var_name = IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(var_info.var_decl));
    size_t var_name_len = strlen(var_name);
    tree var_string = build_string(var_name_len + 1, var_name);

    TREE_TYPE(var_string) = build_array_type(char_type_node,
                                             build_index_type(size_int(var_name_len)));

    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build1(ADDR_EXPR, TREE_TYPE(info_fields), var_string));
    info_fields = DECL_CHAIN(info_fields);

    /* Pointer to variable as a (void *) */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build1(ADDR_EXPR, build_pointer_type(void_type_node), 
                                  var_info.var_decl));
    info_fields = DECL_CHAIN(info_fields);

    /* width of referenced varibale */
    int width = int_size_in_bytes(TREE_TYPE(var_info.var_decl));
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                            build_int_cstu(TREE_TYPE(info_fields), width));
    info_fields = DECL_CHAIN(info_fields);

    /* materializd value -- Filled by Runtime System */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build_int_cstu(TREE_TYPE(info_fields), 0));
    info_fields = DECL_CHAIN(info_fields);

    /* n_functions -- Filled by Runtime System */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build_int_cstu(TREE_TYPE(info_fields), 0));
    info_fields = DECL_CHAIN(info_fields);

    /* functions -- Filled by Runtime System */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields, null_pointer_node);
    info_fields = DECL_CHAIN(info_fields);

    gcc_assert(!info_fields); // All fields are filled

    return build_constructor(ctx->var_type, obj);
}


static void build_info_mvfn_type(tree info_mvfn_type, tree info_assignment_ptr_type)
{
    /*
      struct __mv_info_mvfn {
        void * mv_function;

        unsigned int n_assignments;
        struct mv_info_assignment * assignments;
      };
    */
    tree field, fields = NULL_TREE;

    /* Pointer to function body */
    RECORD_FIELD(build_pointer_type (void_type_node));

    /* n_assignments */
    RECORD_FIELD(get_mv_unsigned_t());

    /* mv_assignments pointer */
    RECORD_FIELD(build_qualified_type(info_assignment_ptr_type, TYPE_QUAL_CONST));

    finish_builtin_struct(info_mvfn_type, "__mv_info_mvfn", fields, NULL_TREE);
}


tree build_info_mvfn(mv_info_mvfn_data &mvfn_info, mv_info_ctx_t *ctx)
{
    vec<constructor_elt, va_gc> *obj = NULL;
    tree info_fields = TYPE_FIELDS(ctx->mvfn_type);

    /* MV Function pointer  as a (void *) */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build1(ADDR_EXPR,
                                  build_pointer_type(void_type_node),
                                                     mvfn_info.mvfn_decl));
    info_fields = DECL_CHAIN(info_fields);

    /* n_assignments */
    unsigned n_assigns = mvfn_info.assignments.size();
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build_int_cstu(TREE_TYPE(info_fields), n_assigns));
    info_fields = DECL_CHAIN(info_fields);

    /* mv_assignments */
    tree assigns_ary = build_array_from_list(mvfn_info.assignments,
                                             ctx->assignment_type,
                                             build_info_assignment,
                                             ctx);
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build1(ADDR_EXPR, ctx->assignment_ptr_type, assigns_ary));

    info_fields = DECL_CHAIN(info_fields);

    gcc_assert(!info_fields); // All fields are filled

    tree ctor = build_constructor(ctx->mvfn_type, obj);

    return ctor;
}


static void build_info_assignment_type(tree info_assignment_type, tree info_var_ptr_type)
{
    /*
      struct __mv_info_assignment {
        struct mv_info_var * variable;
        int lower_bound;
        int upper_bound;
      };
    */

    tree field, fields = NULL_TREE;

    /* Variable this assignment refers to */
    RECORD_FIELD(info_var_ptr_type);

    /* lower limit */
    RECORD_FIELD(get_mv_unsigned_t());

    /* upper limit */
    RECORD_FIELD(get_mv_unsigned_t());

    finish_builtin_struct(info_assignment_type, "__mv_info_assignment", fields, NULL_TREE);
}


tree build_info_assignment(mv_info_assignment_data &assign_info, mv_info_ctx_t *ctx)
{
    vec<constructor_elt, va_gc> *obj = NULL;
    tree info_fields = TYPE_FIELDS(ctx->assignment_type);

    /* Pointer to actual variable. This is replaced by the runtime
       system */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build1(ADDR_EXPR,
                                  build_pointer_type(void_type_node),
                                  assign_info.var_decl));
    info_fields = DECL_CHAIN(info_fields);

    /* lower limit */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build_int_cstu(TREE_TYPE(info_fields),
                                          assign_info.lower_limit));
    info_fields = DECL_CHAIN(info_fields);

    /* upper limit */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build_int_cstu(TREE_TYPE(info_fields),
                                          assign_info.upper_limit));
    info_fields = DECL_CHAIN(info_fields);

    gcc_assert(!info_fields); // All fields are filled

    tree ctor = build_constructor(ctx->mvfn_type, obj);

    return ctor;
}


#undef RECORD_FIELD

static void build_types(mv_info_ctx_t *t)
{
    t->info_type = lang_hooks.types.make_type(RECORD_TYPE);
    t->fn_type = lang_hooks.types.make_type(RECORD_TYPE);
    t->var_type = lang_hooks.types.make_type(RECORD_TYPE);
    t->mvfn_type = lang_hooks.types.make_type(RECORD_TYPE);
    t->assignment_type = lang_hooks.types.make_type(RECORD_TYPE);

    t->info_ptr_type = build_pointer_type(t->info_type);
    t->fn_ptr_type = build_pointer_type(t->fn_type);
    t->var_ptr_type = build_pointer_type(t->var_type);
    t->mvfn_ptr_type = build_pointer_type(t->mvfn_type);
    t->assignment_ptr_type = build_pointer_type(t->assignment_type);

    build_info_type(t->info_type, t->var_ptr_type, t->fn_ptr_type);
    build_info_fn_type(t->fn_type, t->mvfn_ptr_type);
    build_info_var_type(t->var_type, t->mvfn_ptr_type);
    build_info_mvfn_type(t->mvfn_type, t->assignment_ptr_type);
    build_info_assignment_type(t->assignment_type, t->var_ptr_type);
}


/*
 * Generate the constructor function to call __gcov_init.
 */
static void build_init_ctor(tree mv_info_ptr_type, tree mv_info_var)
{
    tree ctor, stmt, init_fn;

    /* Build a decl for __gcov_init.  */
    init_fn = build_function_type_list(void_type_node, mv_info_ptr_type, NULL);
    init_fn = build_decl(BUILTINS_LOCATION, FUNCTION_DECL,
                         get_identifier("__multiverse_init"), init_fn);
    TREE_PUBLIC(init_fn) = 1;
    DECL_EXTERNAL(init_fn) = 1;
    DECL_ASSEMBLER_NAME(init_fn);

    /* Generate a call to __multiverse_init(&mv_info).  */
    ctor = NULL;
    stmt = build_fold_addr_expr(mv_info_var);
    stmt = build_call_expr(init_fn, 1, stmt);
    append_to_statement_list(stmt, &ctor);

    /* Generate a constructor to run it.  */
    cgraph_build_static_cdtor('I', ctor, DEFAULT_INIT_PRIORITY);
}


static bool mv_info_init_done = false;

static void mv_info_init(void *event_data, void *data)
{
    debug_print("mv: start mv_info\n");
    build_types((mv_info_ctx_t *) data);
}



static bool mv_info_finish_done = false;

static void mv_info_finish(void *event_data, void *data)
{
    debug_print("mv: finish mv_info\n");
    mv_info_ctx_t *ctx = (mv_info_ctx_t *) data;
    tree info_var = build_var(ctx->info_type, "__mv_info_");

    /* Create the constructor for the top-level mv_info object */
    vec<constructor_elt, va_gc> *obj = NULL;
    tree info_fields = TYPE_FIELDS(ctx->info_type);

    /* Version ident */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build_int_cstu(TREE_TYPE(info_fields), MV_VERSION));
    info_fields = DECL_CHAIN(info_fields);

    /* next -- NULL; Filled by Runtime System */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields, null_pointer_node);
    info_fields = DECL_CHAIN(info_fields);

    /* n_variables */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build_int_cstu(TREE_TYPE(info_fields),
                                          ctx->variables.size()));
    info_fields = DECL_CHAIN(info_fields);

    /* variables -- NULL */
    tree var_ary = build_array_from_list(ctx->variables, ctx->var_type,
                                         build_info_var, ctx);
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build1(ADDR_EXPR, ctx->var_ptr_type, var_ary));
    info_fields = DECL_CHAIN(info_fields);

    /* n_functions */
    unsigned n_functions = ctx->functions.size();
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build_int_cstu(TREE_TYPE(info_fields),
                                          n_functions));
    info_fields = DECL_CHAIN(info_fields);

    /* functions -- NULL */
    tree fn_ary = build_array_from_list(ctx->functions, ctx->fn_type,
                                        build_info_fn, ctx);
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                          build1(ADDR_EXPR, ctx->fn_ptr_type, fn_ary));
    info_fields = DECL_CHAIN(info_fields);

    gcc_assert(!info_fields); // All fields are filled

    tree ctor = build_constructor(ctx->info_type, obj);

    /* And Initialize a variable with it */
    DECL_INITIAL(info_var) = ctor;

    varpool_node::finalize_decl(info_var);

    // Vall __multiverse_init() on program startup
    build_init_ctor(ctx->info_ptr_type, info_var);
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

    clone_func = DECL_STRUCT_FUNCTION(clone);
    push_cfun(clone_func);
    mv_info_mvfn_data mvfn;
    mvfn.parent_fn_data = &fn_info;
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

    if (!is_cloneable_function(cfun->decl)) {
#ifdef DEBUG
        fprintf(stderr, ".... skipping, it's not multiverseable\n");
        fprintf(stderr, "************************************************************\n\n");
#endif
        return 0;
    }

    std::set<tree> mv_vars;
    std::set<tree> mv_blacklist;
    basic_block bb;
    /* Iterate of each basic block in current function. */
    FOR_EACH_BB_FN(bb, cfun) {
        /* Iterate over each GIMPLE statement. */
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
 * Initialization function of this plugin: the very heart & soul.
 */
int plugin_init(struct plugin_name_args *info, struct plugin_gcc_version *version)
{
#ifdef DEBUG
    fprintf(stderr, "---- Initializing the multiverse GCC plugin.\n");
#endif

    const char * plugin_name = info->base_name;
    struct register_pass_info find_mv_vars_info;

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

    // register the multiverse GIMPLE pass
    find_mv_vars_info.pass = make_find_mv_vars_pass();
    find_mv_vars_info.reference_pass_name = "early_optimizations";
    find_mv_vars_info.ref_pass_instance_number = 0;
    find_mv_vars_info.pos_op = PASS_POS_INSERT_AFTER; // AFTER => more optimized code
    register_callback(plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &find_mv_vars_info);

    // Finish off the generation of multiverse info
    register_callback(plugin_name, PLUGIN_FINISH_UNIT, mv_info_finish, &mv_info_ctx);

    return 0;
}

