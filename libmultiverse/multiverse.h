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
struct mv_info_callsite;
struct mv_patchpoint;

struct mv_info_var_inst;
struct mv_info_fn_inst;
struct mv_info_callsite_inst;

typedef __UINT_LEAST32_TYPE__ mv_value_t;


struct mv_info_assignment {
    union {
        // The GCC plugin generates a variable_location pointer which will be
        // used by the runtime system to determine and save the mv_info_var that
        // belongs to the pointer.
        void *location;
        struct mv_info_var *info;
    } variable;
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


struct mv_info_mvfn {
    // static
    void *function_body;             // A pointer to the mvfn's function body
    unsigned int n_assignments;      // The mvfn's variable assignments
    struct mv_info_assignment *assignments;

    // runtime
    int type;                        // This is be interpreted as mv_type_t
                                     // (declared as integer to ensure correct size)
    mv_value_t constant;
};


struct mv_info_fn {
    // static
    char *const name;       // Functions's symbol name
    void *function_body;    // A pointer to the original (generic) function body
    int n_mv_functions;     // Specialized multiverse variant functions of this function
    struct mv_info_mvfn *mv_functions;
    // In case that the mv_info_fn represents a multiversed function pointer
    // (instead of a multiversed function) the field n_mv_functions is set to -1.
    // In this case mv_functions points to a single mvfn descriptor that is
    // temporarily used for each function that is assigned to the function pointer.

    // runtime
    struct mv_patchpoint *patchpoints_head;  // Patchpoints as linked list TODO: arch-specific
    struct mv_info_mvfn *active_mvfn; // The currently active mvfn
};

/* Short expansion of the original multiverse struct for better traceability. */
struct mv_info_fn_xt {
    struct mv_info_fn_xt *next;
    struct mv_info_fn **mv_info_fn; /* The original multiverse function information. */
    size_t mv_info_fn_len; /* The length of the original multiverse fn information list. */
    const char *xt_name; /* The name of the instance of this multiverse fn struct. Can be the module name for a module for example. */
};

struct mv_info_fn_ref {
    struct mv_info_fn_ref *next;
    struct mv_info_fn *fn;
};


struct mv_info_callsite {
    // static
    void *function_body;
    void *call_label;
};

/* Short expansion of the original multiverse struct for better traceability. */
struct mv_info_callsite_xt {
    struct mv_info_callsite_xt *next;
    struct mv_info_callsite **mv_info_callsite; /* The original multiverse callsite information. */
    size_t mv_info_callsite_len; /* The length of the original multiverse callsite information list. */
    const char *xt_name; /* The name of the instance of this multiverse callsite struct. Can be the module name of a module for example. */
};

struct mv_info_var {
    // static
    char *const name;                // Variable's symbol name
    void *variable_location;         // A pointer to the variable
    union {
        unsigned int info;
        struct {
            unsigned int
                variable_width : 4,  // Width of the variable in bytes
                reserved       : 25, // Currently not used
                flag_tracked   : 1,  // Determines if the variable is tracked
                flag_signed    : 1,  // Determines if the variable is signed
                flag_bound     : 1;  // 1 if the variable is bound, 0 if not
                                     // -> this flag is mutable
        };
    };

    // runtime
    struct mv_info_fn_ref *functions_head; // Functions referening this variable
};

/* Short expansion of the original multiverse struct for better traceability. */
struct mv_info_var_xt {
    struct mv_info_var_xt *next;
    struct mv_info_var **mv_info_var; /* The original multiverse variable information. */
    size_t mv_info_var_len; /* The length of the original multiverse variable information list. */
    const char *xt_name; /* The name of the instance of this multiverse var struct. Can be the module name for a module for example. */
};

/* Generic version of extension structs for simple non-type related memory operations. */
struct mv_info_gen_xt {
    struct mv_info_gen_xt *next;
    void **mv_info_entity_list;
    size_t mv_info_entity_list_len;
    const char *xt_name;
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
