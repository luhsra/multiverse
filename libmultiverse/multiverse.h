#ifndef __MULTIVERSE_H
#define __MULTIVERSE_H

struct mv_info_var;
struct mv_info_mvfn;
struct mv_info_fn;
struct mv_info;
struct mv_info_callsite;
struct mv_patchpoints;

typedef unsigned int mv_value_t;

struct mv_info_assignment {
    struct mv_info_var * variable; // Given as a variable_location pointer
    mv_value_t lower_bound;
    mv_value_t upper_bound;
};

struct mv_info_mvfn {
    void * function_body;
    unsigned int n_assignments;
    struct mv_info_assignment * assignments;
};

struct mv_info_fn_extra {
    unsigned int n_patchpoints;
    struct mv_patchpoint *patchpoints;
    struct mv_info_mvfn *active_mvfn;
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

struct mv_info_callsite {
    void *function_body;
    void *call_label;
};


struct mv_info_var_extra {
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
void multiverse_dump_info(FILE*);


struct mv_info_fn  *  multiverse_info_fn(void * function_body);
struct mv_info_var *  multiverse_info_var(void * variable_location);

/*!
  \brief Interate over all multiverses and commit the current state

  \return int
      >= 0: number of changes functions
      <  0: an error occured during the operation
*/
int multiverse_commit();
int multiverse_commit_info_fn(struct mv_info_fn *);
int multiverse_commit_fn(void * function_body);

int multiverse_commit_info_var(struct mv_info_var *);
int multiverse_commit_var(void * var_location);


#endif
