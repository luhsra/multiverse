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


/* A counter to generate unique var names. */
static int name_counter = 0;


/*
 * Returns a static variable declaration for an array consisting of 'elements'
 * that can be constructed by the function '*build_obj'.
 * The returned declaration is not yet finalized (see varpool_node::finalize_decl)
 * and thus not yet not present in the output artifact.
 */
template<typename Seq, typename T>
static tree get_array_decl(const char *name,
                           Seq &elements,
                           tree element_type,
                           tree (*build_obj)(T&, multiverse_info_types&),
                           multiverse_info_types &types)
{
    unsigned int ary_size = 0;
    vec<constructor_elt, va_gc> *ary_ctor = NULL;
    for (auto &data : elements) {
        tree obj = build_obj(data, types);
        // The generator has decided to not emit a descriptor for this element
        if (obj == NULL_TREE)
            continue;
        CONSTRUCTOR_APPEND_ELT(ary_ctor, NULL, obj);
        ary_size++;
    }

    if (ary_size == 0)
        return NULL_TREE;

    // TODO: some of them may be TYPE_QUAL_CONST
    tree ary_type = build_array_type(build_qualified_type(element_type, TYPE_UNQUALIFIED),
                                     build_index_type(size_int(ary_size-1)));

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


/*
 * Builds a static array consisting of 'elements' that can be constructed by the
 * function '*build_obj'.  The array gets a unique name.
 */
template<typename Seq, typename T>
static tree build_array(Seq &elements,
                        tree element_type,
                        tree (*build_obj)(T&, multiverse_info_types&),
                        multiverse_info_types &types)
{
    auto name = std::string("__multiverse_ary_") + std::to_string(name_counter++);
    tree ary = get_array_decl(name.c_str(), elements, element_type, build_obj, types);
    if (ary)
        set_decl_section_name(ary, "__multiverse_data_");

    if (ary != NULL_TREE)
        varpool_node::finalize_decl(ary);
    return ary;
}


/*
 * Builds a static array consisting of 'elements' that can be constructed by the
 * function '*build_obj'.  The array is placed in the specified section.
 */
template<typename Seq, typename T>
static void build_section_array(const char *section_name,
                                Seq &elements,
                                tree element_type,
                                tree (*build_obj)(T&, multiverse_info_types&),
                                multiverse_info_types& types) {
    // Only create the array if necessary (and thus the special section for this
    // compilation unit).

    auto name = std::string(section_name) + "ary_";
    tree ary = get_array_decl(name.c_str(), elements, element_type, build_obj, types);

    if (ary != NULL_TREE) {
        // Set the smallest possible alignment.  The sections of all compilation
        // units will be merged during linking and will be accessed as a single
        // array by the multiverse runtime library.
        SET_DECL_ALIGN(ary, 0);
        DECL_USER_ALIGN(ary) = 1;
        set_decl_section_name(ary, section_name);

        varpool_node::finalize_decl(ary);
    }
}


static tree
build_info_assignment(var_assign_t &assign_info, multiverse_info_types &types)
{
    vec<constructor_elt, va_gc> *obj = NULL;
    tree info_fields = TYPE_FIELDS(types.assignment_type);

    /* Pointer to actual variable. This is replaced by the runtime
       system */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build1(ADDR_EXPR,
                                  build_pointer_type(void_type_node),
                                  assign_info.variable->decl()));
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

    tree ctor = build_constructor(types.mvfn_type, obj);

    return ctor;
}


static tree build_info_mvfn(mvfn_t &mvfn_info, multiverse_info_types &types)
{
    vec<constructor_elt, va_gc> *obj = NULL;
    tree info_fields = TYPE_FIELDS(types.mvfn_type);

    /* MV Function pointer  as a (void *) */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build1(ADDR_EXPR,
                                  build_pointer_type(void_type_node),
                                  mvfn_info.decl()));
    info_fields = DECL_CHAIN(info_fields);

    /* n_assignments */
    unsigned n_assigns = mvfn_info.assignments.size();
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build_int_cstu(TREE_TYPE(info_fields), n_assigns));
    info_fields = DECL_CHAIN(info_fields);

    /* mv_assignments */
    tree assigns_ary = build_array(mvfn_info.assignments, types.assignment_type,
                                   build_info_assignment, types);
    if (assigns_ary != NULL_TREE)
        CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                               build1(ADDR_EXPR, types.assignment_ptr_type,
                                      assigns_ary));

    info_fields = DECL_CHAIN(info_fields);

    /* Fields initialized by the runtime system */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build_int_cstu(TREE_TYPE(info_fields), 0));
    info_fields = DECL_CHAIN(info_fields);
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build_int_cstu(TREE_TYPE(info_fields), 0));
    info_fields = DECL_CHAIN(info_fields);

    gcc_assert(!info_fields); // All fields are filled

    tree ctor = build_constructor(types.mvfn_type, obj);

    return ctor;
}


static tree build_info_fn(func_t &fn_info, multiverse_info_types &types)
{
    /* When a function is defined only as extern, we ignore it */
    if (!fn_info.is_definition) {
        return NULL_TREE;
    }

    /* Create the constructor for the top-level mv_info object */
    vec<constructor_elt, va_gc> *obj = NULL;
    tree info_fields = TYPE_FIELDS(types.fn_type);

    /* Name of function */
    const char *fn_name = fn_info.name();
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
                                  fn_info.decl()));

    info_fields = DECL_CHAIN(info_fields);

    /* n_mv_functions */
    unsigned n_mv_functions = fn_info.mv_functions.size();
    CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                           build_int_cstu(TREE_TYPE(info_fields), n_mv_functions));

    info_fields = DECL_CHAIN(info_fields);

    /* mv_functions -- */
    tree mvfn_ary = build_array(fn_info.mv_functions, types.mvfn_ptr_type,
                                build_info_mvfn, types);
    if (mvfn_ary != NULL_TREE)
        CONSTRUCTOR_APPEND_ELT(obj, info_fields,
                               build1(ADDR_EXPR, types.mvfn_ptr_type, mvfn_ary));
    info_fields = DECL_CHAIN(info_fields);

    /* Fields initialized by the runtime system */
    CONSTRUCTOR_APPEND_ELT(obj, info_fields, null_pointer_node);
    info_fields = DECL_CHAIN(info_fields);
    CONSTRUCTOR_APPEND_ELT(obj, info_fields, null_pointer_node);
    info_fields = DECL_CHAIN(info_fields);

    gcc_assert(!info_fields); // All fields are filled

    return build_constructor(types.fn_type, obj);
}


static tree build_info_var(variable_t &var_info, multiverse_info_types &types)
{
    /* When a variable is defined only as extern, we ignore it */
    if (!var_info.is_definition) {
        return NULL_TREE;
    }

    vec<constructor_elt, va_gc> *obj = NULL;
    tree info_fields = TYPE_FIELDS(types.var_type);

    /* name of variable */
    const char *var_name = var_info.name();
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
                                  var_info.decl()));
    info_fields = DECL_CHAIN(info_fields);

    /* information about the variable */
    tree type = TREE_TYPE(var_info.decl());
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

    return build_constructor(types.var_type, obj);
}


static tree build_info_callsite(callsite_t &cs_info, multiverse_info_types &types)
{
    vec<constructor_elt, va_gc> *obj = NULL;
    tree info_fields = TYPE_FIELDS(types.callsite_type);

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

    return build_constructor(types.callsite_type, obj);
}


static void build_info(multiverse_context *ctx, multiverse_info_types &types) {
    // Version ident
    // TODO: MV_VERSION ???

    // Build the variables section.
    build_section_array("__multiverse_var_", ctx->variables, types.var_type,
                        build_info_var, types);

    // Build the functions section.
    build_section_array("__multiverse_fn_", ctx->functions, types.fn_type,
                        build_info_fn, types);

    // Build the callsites section.
    build_section_array("__multiverse_callsite_", ctx->callsites,
                        types.callsite_type, build_info_callsite, types);
}


void mv_info_init(void *event_data, void *data)
{
    (void) event_data;
    (void) data;
    (void) gcc_version;
}

void mv_info_finish(void *event_data, void *data)
{
    (void) event_data;
    multiverse_context *ctx = (multiverse_context*) data;

    auto types = multiverse_info_types::build();
    build_info(ctx, types);
}
