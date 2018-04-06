#include "gcc-common.h"
#include "multiverse.h"


#define RECORD_FIELD(type)                                       \
    field = build_decl(BUILTINS_LOCATION, FIELD_DECL, NULL_TREE, \
                       (type)); \
    DECL_CHAIN(field) = fields; \
    fields = field;


static tree get_mv_unsigned_t(void)
{
    machine_mode mode = smallest_mode_for_size(32, MODE_INT);
    return lang_hooks.types.type_for_mode(mode, true);
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


static void build_info_var_type(tree info_variable_type)
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


static void build_info_assignment_type(tree info_assignment_type)
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


multiverse_info_types multiverse_info_types::build()
{
    (void) gcc_version;
    tree fn_type = lang_hooks.types.make_type(RECORD_TYPE);
    tree var_type = lang_hooks.types.make_type(RECORD_TYPE);
    tree mvfn_type = lang_hooks.types.make_type(RECORD_TYPE);
    tree assignment_type = lang_hooks.types.make_type(RECORD_TYPE);
    tree callsite_type = lang_hooks.types.make_type(RECORD_TYPE);

    tree fn_ptr_type = build_pointer_type(fn_type);
    tree var_ptr_type = build_pointer_type(var_type);
    tree mvfn_ptr_type = build_pointer_type(mvfn_type);
    tree assignment_ptr_type = build_pointer_type(assignment_type);
    tree callsite_ptr_type = build_pointer_type(callsite_type);

    build_info_fn_type(fn_type, mvfn_ptr_type);
    build_info_var_type(var_type);
    build_info_mvfn_type(mvfn_type, assignment_ptr_type);
    build_info_assignment_type(assignment_type);
    build_info_callsite_type(callsite_type);

    return multiverse_info_types(fn_type, fn_ptr_type,
                                 var_type, var_ptr_type,
                                 mvfn_type, mvfn_ptr_type,
                                 assignment_type, assignment_ptr_type,
                                 callsite_type, callsite_ptr_type);
}


multiverse_info_types::multiverse_info_types(tree fn_type, tree fn_ptr_type,
                                             tree var_type, tree var_ptr_type,
                                             tree mvfn_type, tree mvfn_ptr_type,
                                             tree assignment_type,
                                             tree assignment_ptr_type,
                                             tree callsite_type,
                                             tree callsite_ptr_type)
    : fn_type(fn_type), fn_ptr_type(fn_ptr_type),
      var_type(var_type), var_ptr_type(var_ptr_type),
      mvfn_type(mvfn_type), mvfn_ptr_type(mvfn_ptr_type),
      assignment_type(assignment_type), assignment_ptr_type(assignment_ptr_type),
      callsite_type(callsite_type), callsite_ptr_type(callsite_ptr_type)
{}
