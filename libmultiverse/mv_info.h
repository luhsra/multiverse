struct mv_info_var;
struct mv_info_mvfn;
struct mv_info_fn;
struct mv_info;

struct mv_info_var_assign {
    struct mv_info_var * variable;
    int lower_bound;
    int upper_bound;
};

struct mv_info_mvfn {
    struct mv_info_fn *function;

    void * mv_function;
    unsigned int n_assignments;
    struct mv_info_var_assign ** assignments;
};

struct mv_info_fn {
    char * const name;

    /* Where is the original function body located in the text
     * segment? */
    void * original_function;

    /* What variants exist for this function */
    unsigned int n_mv_functions;
    struct mv_info_mvfn ** mv_functions;
};

struct mv_info_var {
    char * const name;
    void * variable;

    int materialized_value;

    unsigned int n_mv_functions;
    struct mv_info_mvfn ** mv_functions;
};

struct mv_info {
    /* This multiverse info record was generated for multiverse
     * version N */
    unsigned int version;

    struct mv_info *next;

    unsigned int n_variables;
    struct mv_info_var **variables;

    unsigned int n_functions;
    struct mv_info_fn ** functions;
};
