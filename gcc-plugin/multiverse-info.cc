#include "gcc-common.h"
#include <tree-iterator.h>
#include <string>
#include "multiverse.h"

typedef multiverse_context::func_t func_t;
typedef multiverse_context::variable_t variable_t;
typedef multiverse_context::var_assign_t var_assign_t;
typedef multiverse_context::mvfn_t mvfn_t;
typedef multiverse_context::callsite_t callsite_t;


#if BUILDING_GCC_VERSION < 7000
#define SET_DECL_ALIGN(VAR, TYPE) DECL_ALIGN(VAR) = TYPE
#endif

static tree build_info_fn(func_t &, multiverse_context *);
static tree build_info_var(variable_t &, multiverse_context *);
static tree build_info_callsite(callsite_t &, multiverse_context *);
static tree build_info_mvfn(mvfn_t &, multiverse_context *);
static tree build_info_assignment(var_assign_t &, multiverse_context *);

static int name_counter = 0;


static tree get_mv_unsigned_t(void)
{
    machine_mode mode = smallest_mode_for_size(32, MODE_INT);
    return lang_hooks.types.type_for_mode(mode, true);
}


static const char *get_source_filename(void) {
    return expand_location(input_location).file;
}


template<typename Seq, typename T>
static tree get_array_decl(const char *name,
                           Seq &elements,
                           tree element_type,
                           tree (*build_obj)(T&, multiverse_context*),
                           multiverse_context *ctx)
{
    // TODO: some of them may be TYPE_QUAL_CONST
    tree ary_type = build_array_type(build_qualified_type(element_type, TYPE_UNQUALIFIED),
                                     build_index_type(size_int(elements.size()-1)));

    vec<constructor_elt, va_gc> *ary_ctor = NULL;
    for (auto &data : elements) {
        tree obj = build_obj(data, ctx);
        CONSTRUCTOR_APPEND_ELT(ary_ctor, NULL, obj);
    }

    tree ary = build_decl(BUILTINS_LOCATION, VAR_DECL, NULL_TREE, ary_type);
    SET_DECL_ASSEMBLER_NAME(ary, get_identifier(name));
    TREE_STATIC(ary) = 1;
    TREE_ADDRESSABLE(ary) = 1;
    DECL_NONALIASED(ary) = 1;
    DECL_VISIBILITY_SPECIFIED(ary) = 1;
    DECL_VISIBILITY(ary) = VISIBILITY_HIDDEN;
    DECL_INITIAL(ary) = build_constructor(ary_type, ary_ctor);

    return ary;
}


template<typename Seq, typename T>
static tree build_array(Seq &elements,
                        tree element_type,
                        tree (*build_obj)(T&, multiverse_context*),
                        multiverse_context *ctx)
{
    if (elements.size() == 0)
        return NULL;

    auto name = std::string("__multiverse_ary_") + std::to_string(name_counter++);
    tree ary = get_array_decl(name.c_str(), elements, element_type, build_obj, ctx);

    varpool_node::finalize_decl(ary);
    return ary;
}


template<typename Seq, typename T>
static tree build_section_array(const char *section_name,
                                Seq &elements,
                                tree element_type,
                                tree (*build_obj)(T&, multiverse_context*),
                                multiverse_context *ctx) {
    if (elements.size() == 0)
        return NULL;

    auto name = std::string(section_name) + get_source_filename();
    tree ary = get_array_decl(name.c_str(), elements, element_type, build_obj, ctx);

    SET_DECL_ALIGN(ary, 0);
    DECL_USER_ALIGN(ary) = 1;
    set_decl_section_name(ary, section_name);

    varpool_node::finalize_decl(ary);
    return ary;
}


#define RECORD_FIELD(type) \
    field = build_decl(BUILTINS_LOCATION, FIELD_DECL, NULL_TREE, \
                       (type)); \
    DECL_CHAIN(field) = fields; \
    fields = field;


static void build_info(multiverse_context *ctx) {
    /* Nothing was multiverse in this translation unit */
    if (ctx->variables.size() == 0
        && ctx->functions.size() == 0
        && ctx->callsites.size() == 0)
        return;

    /* Version ident */
    // TODO: MV_VERSION ???

    /* variables */
    build_section_array("__multiverse_var_", ctx->variables, ctx->var_type,
                        build_info_var, ctx);

    /* functions */
    build_section_array("__multiverse_fn_", ctx->functions, ctx->fn_type,
                        build_info_fn, ctx);

    /* callsites */
    build_section_array("__multiverse_callsite_", ctx->callsites,
                        ctx->callsite_type, build_info_callsite, ctx);
}


static void build_info_fn_type(tree info_fn_type, tree info_mvfn_ptr_type)
{
    /*
      struct __mv_info_fn {
        char * const const name;
        void * original_function;

        unsigned int n_mv_functions;
        struct mv_info_mvfn ** mv_functions;

        struct mv_patchpoint * patchpoints_head;
        struct mv_info_mvfn * active_mvfn;
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

    /* Fields initialized by the runtime system */
    /* patchpoints_head */
    RECORD_FIELD(build_pointer_type(void_type_node));

    /* active_mvfn */
    RECORD_FIELD(build_pointer_type(void_type_node));

    finish_builtin_struct(info_fn_type, "__mv_info_fn", fields, NULL_TREE);
}


static tree build_info_fn(func_t &fn_info, multiverse_context *ctx)
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
    tree mvfn_ary = build_array(fn_info.mv_functions, ctx->mvfn_ptr_type,
                                build_info_mvfn, ctx);
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build1(ADDR_EXPR, ctx->mvfn_ptr_type, mvfn_ary));
    info_fields = DECL_CHAIN(info_fields);

    /* Fields initialized by the runtime system */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields, null_pointer_node);
    info_fields = DECL_CHAIN(info_fields);
    CONSTRUCTOR_APPEND_ELT(obj, info_fields, null_pointer_node);
    info_fields = DECL_CHAIN(info_fields);

    gcc_assert(!info_fields); // All fields are filled

    return build_constructor(ctx->fn_type, obj);
}


static void build_info_variable_type(tree info_variable_type)
{
    /*
      struct __mv_info_var {
        char * const name;

        void * variable;
        struct {
              unsigned int
                 variable_width  : 4,
                 reserved        : 25,
                 flag_signed     : 1,
                 flag_tracked    : 1,
                 flag_bound      : 1,
        } __attribute__((packed));

        unsigned int n_functions;
        struct mv_info_fn **functions;
      };
    */
    tree field, fields = NULL_TREE;

    /* Name of variable */
    RECORD_FIELD(build_pointer_type(build_qualified_type(char_type_node, TYPE_QUAL_CONST)));

    /* Pointer to variable */
    RECORD_FIELD(build_pointer_type(void_type_node));

    /* info - uint32_t */
    RECORD_FIELD(uint32_type_node);

    /* Fields initialized by the runtime system */
    /* n_functions */
    RECORD_FIELD(integer_type_node);
    /* functions */
    RECORD_FIELD(build_pointer_type(void_type_node));

    finish_builtin_struct(info_variable_type, "__mv_info_var", fields, NULL_TREE);
}


static tree build_info_var(variable_t &var_info, multiverse_context *ctx)
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

    /* information about the variable */
    tree type = TREE_TYPE(var_info.var_decl);
    int width = int_size_in_bytes(type);
    int flag_signed = !TYPE_UNSIGNED(type);
    int flag_tracked = !!var_info.tracked;
    gcc_assert(width < 16);
    uint32_t info = ((!flag_tracked) << 31) | (flag_signed << 30) |
                    (flag_tracked << 29) | width;
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                            build_int_cstu(TREE_TYPE(info_fields), info));
    info_fields = DECL_CHAIN(info_fields);

    /* Fields initialized by the runtime system */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build_int_cstu(TREE_TYPE(info_fields), 0));
    info_fields = DECL_CHAIN(info_fields);
    CONSTRUCTOR_APPEND_ELT(obj, info_fields, null_pointer_node);
    info_fields = DECL_CHAIN(info_fields);


    gcc_assert(!info_fields); // All fields are filled

    return build_constructor(ctx->var_type, obj);
}


static void build_info_callsite_type(tree info_variable_type)
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

    finish_builtin_struct(info_variable_type, "__mv_info_callsite", fields, NULL_TREE);
}

static tree build_info_callsite(callsite_t &cs_info, multiverse_context *ctx)
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

        int type;
        mv_value_t constant;
      };
    */
    tree field, fields = NULL_TREE;

    /* Pointer to function body */
    RECORD_FIELD(build_pointer_type(void_type_node));

    /* n_assignments */
    RECORD_FIELD(get_mv_unsigned_t());

    /* mv_assignments pointer */
    RECORD_FIELD(build_qualified_type(info_assignment_ptr_type, TYPE_QUAL_CONST));

    /* Fields initialized by the runtime system */
    /* normal integer */
    RECORD_FIELD(integer_type_node);

    /* mv value integer */
    RECORD_FIELD(get_mv_unsigned_t());

    finish_builtin_struct(info_mvfn_type, "__mv_info_mvfn", fields,
                          NULL_TREE);
}


static tree build_info_mvfn(mvfn_t &mvfn_info, multiverse_context *ctx)
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
    tree assigns_ary = build_array(mvfn_info.assignments, ctx->assignment_type,
                                   build_info_assignment, ctx);
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build1(ADDR_EXPR, ctx->assignment_ptr_type, assigns_ary));

    info_fields = DECL_CHAIN(info_fields);

    /* Fields initialized by the runtime system */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build_int_cstu(TREE_TYPE(info_fields), 0));
    info_fields = DECL_CHAIN(info_fields);
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build_int_cstu(TREE_TYPE(info_fields), 0));
    info_fields = DECL_CHAIN(info_fields);

    gcc_assert(!info_fields); // All fields are filled

    tree ctor = build_constructor(ctx->mvfn_type, obj);

    return ctor;
}


static void build_info_assignment_type(tree info_assignment_type, tree info_var_ptr_type)
{
    /*
      struct __mv_info_assignment {
        void *variable_location;
        int lower_bound;
        int upper_bound;
      };
    */

    tree field, fields = NULL_TREE;

    /* Variable this assignment refers to */
    RECORD_FIELD(build_pointer_type(void_type_node));

    /* lower limit */
    RECORD_FIELD(get_mv_unsigned_t());

    /* upper limit */
    RECORD_FIELD(get_mv_unsigned_t());

    finish_builtin_struct(info_assignment_type, "__mv_info_assignment",
                          fields, NULL_TREE);
}


static tree
build_info_assignment(var_assign_t &assign_info, multiverse_context *ctx)
{
    vec<constructor_elt, va_gc> *obj = NULL;
    tree info_fields = TYPE_FIELDS(ctx->assignment_type);

    /* Pointer to actual variable. This is replaced by the runtime
       system */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build1(ADDR_EXPR,
                                  build_pointer_type(void_type_node),
                                  assign_info.variable->var_decl));
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

static void build_types(multiverse_context *t)
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


    build_info_fn_type(t->fn_type, t->mvfn_ptr_type);
    build_info_variable_type(t->var_type);
    build_info_mvfn_type(t->mvfn_type, t->assignment_ptr_type);
    build_info_assignment_type(t->assignment_type, t->var_ptr_type);
    build_info_callsite_type(t->callsite_type);
}


void mv_info_init(void *event_data, void *data)
{
    (void) event_data;
    (void) gcc_version;

    build_types((multiverse_context *) data);
}

void mv_info_finish(void *event_data, void *data)
{
    (void) event_data;
    multiverse_context *ctx = (multiverse_context *) data;
    build_info(ctx);
}
