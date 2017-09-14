/**
   @defgroup libmultiverse

   foobar
*/
/**
   @file
   @brief User API for Multiverse
   @ingroup libmultiverse
*/


#ifndef __MULTIVERSE_H
#define __MULTIVERSE_H

#ifdef __cplusplus
extern "C" {
#endif

struct mv_info_var;
struct mv_info_mvfn;
struct mv_info_fn;
struct mv_info;
struct mv_info_callsite;
struct mv_patchpoint;

typedef unsigned int mv_value_t;

struct mv_info_assignment {
    struct mv_info_var * variable; // Given as a variable_location pointer
    mv_value_t lower_bound;
    mv_value_t upper_bound;
};

typedef enum {
    MVFN_TYPE_NONE,
    MVFN_TYPE_NOP,
    MVFN_TYPE_CONSTANT,
    MVFN_TYPE_CLI,
    MVFN_TYPE_STI,
} mvfn_type_t;

struct mv_info_mvfn_extra {
    mvfn_type_t type;
    mv_value_t constant;
};


struct mv_info_mvfn {
    void * function_body;
    unsigned int n_assignments;
    struct mv_info_assignment * assignments;
    union {
        void *data;
        struct mv_info_mvfn_extra *extra;
    };
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
    char bound;
};


struct mv_info_var {
    char * const  name;
    void *        variable_location;

    union {
        unsigned int info;
        struct {
            unsigned int
                variable_width  : 4,
                reserved        : 26,
                flag_tracked    : 1,
                flag_signed     : 1;
        };
    };

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

int multiverse_init(void);
void multiverse_dump_info(void);


struct mv_info_fn  *  multiverse_info_fn(void * function_body);
struct mv_info_var *  multiverse_info_var(void * variable_location);

/**
  @brief Iterate over all multiverse functions and commit

  This function iterates over all functions for which a multiverse
  exists. It selects the best fitting multiverse instance (mvfn) and
  installs it as the currently active variant in the code. This
  installation is done by binary patching the text segment.

  The function returns the number of changed functions, or -1 on
  error.

  @return number of changed functions or -1 on error
*/
int multiverse_commit(void);

/**
  @brief commit a single multiverse function
  @param fn info object identifying the function

  Like multiverse_commit, but operates only on a single function. All
  other functions of the program remain in the current state.

  The function returns the number of changed functions, or -1 on
  error.

  @return number of changed functions or -1 on error
  @sa multiverse_commit, multiverse_commit_fn
*/
int multiverse_commit_info_fn(struct mv_info_fn *fn);

/**
  @brief commit a single multiverse function
  @param function_body the function pointer to the function

  @return number of changed functions or -1 on error
  @sa multiverse_commit

  Like multiverse_commit, but operates only on a single function. All
  other functions of the program remain in the current state.

  The function returns 1, if a different mvfn was selected. 0 if
  nothing changes, and -1 on error.

  @verbatim
  void __attribute__((multiverse)) foo() {...}
  ...
  multiverse_commit_fn(&foo);
  @endverbatim

*/
int multiverse_commit_fn(void * function_body);

/**
   @brief commit all functions that reference a variable
   @param var info object referencing the function

   @return number of changed functions or -1 on error
   @sa multiverse_commit, multiverse_commit_var

   Like multiverse_commit, but operates on all functions that
   reference the given variable.

   The function returns 1, if a different mvfn was selected. 0 if
   nothing changes, and -1 on error.

*/
int multiverse_commit_info_refs(struct mv_info_var *var);

/**
   @brief commit all functions that reference a variable
   @param var_location pointer to the multiverse variable

   @return number of changed functions or -1 on error
   @sa multiverse_commit, multiverse_commit_info_var

   Like multiverse_commit, but operates on all functions that
   reference the given variable.

   The function returns 1, if a different mvfn was selected. 0 if
   nothing changes, and -1 on error.

   @verbatim
   int __attribute__((multiverse)) config;
   ...
   multiverse_commit_refs(&config);
   @endverbatim
*/
int multiverse_commit_refs(void * var_location);


/**
   @brief commit a single multiverse function
   @param fn info object identifying the function

   @return number of changed functions or -1 on error
   @sa multiverse_commit, multiverse_commit_fn
*/

int multiverse_revert_info_fn(struct mv_info_fn* fn);

/**
   @brief Revert multiverse code modifications for one function
   @param function_body pointer to the function body

   Calling this function reverts all code modification that were done
   on the given function.

   @return number of changes or -1 on error
*/
int multiverse_revert_fn(void* function_body);


/**
   @brief commit all functions that reference a variable
   @param var info object referencing the function

   @return number of changed functions or -1 on error
   @sa multiverse_revert, multiverse_revert_var

   Like multiverse_revert, but operates on all functions that
   reference the given variable.

   The function returns 1, if a different mvfn was selected. 0 if
   nothing changes, and -1 on error.

*/
int multiverse_revert_info_refs(struct mv_info_var *var);

/**
   @brief commit all functions that reference a variable
   @param var_location pointer to the multiverse variable

   @return number of changed functions or -1 on error
   @sa multiverse_revert, multiverse_revert_info_var

   Like multiverse_revert, but operates on all functions that
   reference the given variable.

   The function returns 1, if a different mvfn was selected. 0 if
   nothing changes, and -1 on an error.

*/
int multiverse_revert_refs(void * var_location);

/**
   @brief Revert all multiverse code modifications

   Calling this function reverts all code modification that were done
   to all multiversed variables.

   @return number of changes functions or -1 on error
*/
int multiverse_revert(void);

/**
   @brief Test whether a function is currently committed to a mulitverse variant.
   @param function_body pointer to the function body

   Test whether the function body of a function is currently under the
   control of multiverse.

   @return bool
*/
int multiverse_is_committed(void* function_body);

/**
   @brief Change the binding state of a variable
   @param var_location pointer to the variable
   @param state the new binding state.
     - 1 if variable should be bound
     - 0 if variable should be unbound
     - -1 to query the binding state

   If a variable is attributed with multiverse("tracked"), the
   variable is not binded by default with multiverse_commit, since its
   binding state is unbound. With this function, we can change the
   binding state.

   This function will only work on "tracked" variables.

   Please note, that the commit operation has to be done explicitly!

   @return new binding state or error
      - 0   if variable is unbound
      - 1   if variable is bound
      - -1  if variable was not marked as "tracked"
*/
int multiverse_bind(void* var_location, int state);


#ifdef __cplusplus
} // extern "C"
#endif
#endif
