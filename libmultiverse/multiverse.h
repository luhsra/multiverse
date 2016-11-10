struct mv_info_var;
struct mv_info_mvfn;
struct mv_info_fn;
struct mv_info;
struct mv_info_callsite;

struct mv_info_assignment {
    struct mv_info_var * variable; // Given as a variable_location pointer
    int lower_bound;
    int upper_bound;
};

struct mv_info_mvfn {
    void * mv_function;
    unsigned int n_assignments;
    struct mv_info_assignment * assignments;
};

struct mv_info_fn_extra {
    unsigned int n_callsites;
    struct mv_info_callsite **callsites;
};

struct mv_info_fn {
    char * const name;

    /* Where is the original function body located in the text
     * segment? */
    void * function_body;

    /* What variants exist for this function */
    unsigned int n_mv_functions;
    struct mv_info_mvfn * mv_functions;

    union {
        void *data;
        struct mv_info_fn_extra * extra;
    };
};

typedef enum  {
    CS_TYPE_NOTFOUND,
    CS_TYPE_INVALID,
    CS_TYPE_X86_CALLQ,
} mv_info_callsite_type;

struct mv_info_callsite {
    struct mv_info_fn *function; // Given as function_body pointer
    union {
        void *label_before;
        mv_info_callsite_type type;
    };
    union {
        void *label_after;
        void *call_insn;
    };
};

struct mv_info_var_extra {
    unsigned long long materialized_value;
    unsigned int n_functions;
    struct mv_info_fn **functions;
};


struct mv_info_var {
    char * const  name;
    void *        variable_location;
    unsigned char variable_width;

    union {
        void *data;
        struct mv_info_var_extra *extra;
    };
};


struct mv_info {
    /* This multiverse info record was generated for multiverse
     * version N */
    unsigned int version;

    struct mv_info *next; // Filled by runtime system

    unsigned int n_variables;
    struct mv_info_var *variables;

    unsigned int n_functions;
    struct mv_info_fn * functions;

    unsigned int n_callsites;
    struct mv_info_callsite * callsites;
};

int multiverse_init();
struct mv_info_fn *   multiverse_fn_info(void * function_body);
struct mv_info_var *  multiverse_var_info(void * variable_location);

void multiverse_dump_info(FILE*);
