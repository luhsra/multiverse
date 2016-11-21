#include "gcc-common.h"
#include <tree-iterator.h>
#include <sstream>
#include "mv-info.h"
#include "multiverse.h"


static tree build_info_fn(mv_info_fn_data &, mv_info_ctx_t *);
static tree build_info_var(mv_info_var_data &, mv_info_ctx_t *);
static tree build_info_callsite(mv_info_callsite_data &, mv_info_ctx_t *);
static tree build_info_mvfn(mv_info_mvfn_data &, mv_info_ctx_t *);
static tree build_info_assignment(mv_info_assignment_data &, mv_info_ctx_t *);


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


template<typename Seq, typename T>
static tree build_array_from_list(Seq &elements,
                                  tree element_type,
                                  tree (*build_obj_ptr)(T&, mv_info_ctx_t*),
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


static void build_info_type(tree info_type,
                            tree info_var_ptr_type,
                            tree info_fn_ptr_type,
                            tree info_callsite_ptr_type)
{
    /*  struct __mv_info {
          unsigned int version;

          struct mv_info *next;

          unsigned int n_variables;
          struct mv_info_var *variables;

          unsigned int n_functions;
          struct mv_info_fn * functions;

          unsigned int n_callsites;
          struct mv_info_callsite * callsites;
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

    /* n_callsites */
    RECORD_FIELD(get_mv_unsigned_t());

    /* struct mv_info_fn pointer pointer */
    RECORD_FIELD(build_qualified_type(info_callsite_ptr_type, TYPE_QUAL_CONST));

    finish_builtin_struct(info_type, "__mv_info", fields, NULL_TREE);
}

static tree build_info(mv_info_ctx_t *ctx) {
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

    /* functions */
    tree fn_ary = build_array_from_list(ctx->functions, ctx->fn_type,
                                        build_info_fn, ctx);
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                          build1(ADDR_EXPR, ctx->fn_ptr_type, fn_ary));
    info_fields = DECL_CHAIN(info_fields);

    /* n_functions */
    unsigned n_callsites = ctx->callsites.size();
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build_int_cstu(TREE_TYPE(info_fields),
                                          n_callsites));
    info_fields = DECL_CHAIN(info_fields);

    /* callsites -- NULL */
    tree callsite_ary = build_array_from_list(ctx->callsites, ctx->callsite_type,
                                        build_info_callsite, ctx);
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build1(ADDR_EXPR, ctx->callsite_ptr_type, callsite_ary));
    info_fields = DECL_CHAIN(info_fields);

    gcc_assert(!info_fields); // All fields are filled

    return build_constructor(ctx->info_type, obj);
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

    /* void * data */
    RECORD_FIELD(build_pointer_type(void_type_node));

    finish_builtin_struct(info_fn_type, "__mv_info_fn", fields, NULL_TREE);
}


static tree build_info_fn(mv_info_fn_data &fn_info, mv_info_ctx_t *ctx)
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

    /* void * data; */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields, null_pointer_node);
    info_fields = DECL_CHAIN(info_fields);

    gcc_assert(!info_fields); // All fields are filled

    return build_constructor(ctx->fn_type, obj);
}


static void build_info_var_type(tree info_var_type)
{
    /*
      struct __mv_info_var {
        char * const name;

        void * variable;
        unsigned char width;

        void * data;
      };
    */
    tree field, fields = NULL_TREE;

    /* Name of variable */
    RECORD_FIELD(build_pointer_type(build_qualified_type(char_type_node, TYPE_QUAL_CONST)));

    /* Pointer to variable */
    RECORD_FIELD(build_pointer_type(void_type_node));

    /* width */
    RECORD_FIELD(unsigned_char_type_node);

    /* void * data */
    RECORD_FIELD(build_pointer_type(void_type_node));

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

    /* void * data Filled by Runtime System */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields, null_pointer_node);
    info_fields = DECL_CHAIN(info_fields);

    gcc_assert(!info_fields); // All fields are filled

    return build_constructor(ctx->var_type, obj);
}


static void build_info_callsite_type(tree info_var_type)
{
    /*
      struct __mv_info_callsite {
        void * callee;
        void * callsite_label;
      };
    */
    tree field, fields = NULL_TREE;

    /* Pointer to callee body */
    RECORD_FIELD(build_pointer_type (void_type_node));

    /* label_before */
    RECORD_FIELD(build_pointer_type (void_type_node));

    finish_builtin_struct(info_var_type, "__mv_info_callsite", fields, NULL_TREE);
}

static tree build_info_callsite(mv_info_callsite_data &cs_info, mv_info_ctx_t *ctx)
{
    vec<constructor_elt, va_gc> *obj = NULL;
    tree info_fields = TYPE_FIELDS(ctx->callsite_type);

    /* called function */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build1(ADDR_EXPR, TREE_TYPE(info_fields),
                                  cs_info.fn_decl));
    info_fields = DECL_CHAIN(info_fields);

    /* label */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build1(ADDR_EXPR, TREE_TYPE(info_fields),
                                  cs_info.callsite_label));
    info_fields = DECL_CHAIN(info_fields);

    gcc_assert(!info_fields); // All fields are filled

    return build_constructor(ctx->callsite_type, obj);
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


static tree build_info_mvfn(mv_info_mvfn_data &mvfn_info, mv_info_ctx_t *ctx)
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


static tree
build_info_assignment(mv_info_assignment_data &assign_info, mv_info_ctx_t *ctx)
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
    t->callsite_type = lang_hooks.types.make_type(RECORD_TYPE);

    t->info_ptr_type = build_pointer_type(t->info_type);
    t->fn_ptr_type = build_pointer_type(t->fn_type);
    t->var_ptr_type = build_pointer_type(t->var_type);
    t->mvfn_ptr_type = build_pointer_type(t->mvfn_type);
    t->assignment_ptr_type = build_pointer_type(t->assignment_type);
    t->callsite_ptr_type = build_pointer_type(t->callsite_type);


    build_info_type(t->info_type, t->var_ptr_type, t->fn_ptr_type, t->callsite_ptr_type);
    build_info_fn_type(t->fn_type, t->mvfn_ptr_type);
    build_info_var_type(t->var_type);
    build_info_mvfn_type(t->mvfn_type, t->assignment_ptr_type);
    build_info_assignment_type(t->assignment_type, t->var_ptr_type);
    build_info_callsite_type(t->callsite_type);
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


void mv_info_init(void *event_data, void *data)
{
    build_types((mv_info_ctx_t *) data);
}

void mv_info_finish(void *event_data, void *data)
{
    mv_info_ctx_t *ctx = (mv_info_ctx_t *) data;
    tree info_var = build_var(ctx->info_type, "__mv_info_");

    // We get a constructor for the object
    tree info_obj = build_info(ctx);

    /* And Initialize a variable with it */
    DECL_INITIAL(info_var) = info_obj;

    varpool_node::finalize_decl(info_var);

    // Vall __multiverse_init() on program startup
    build_init_ctor(ctx->info_ptr_type, info_var);
}
