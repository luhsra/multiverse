#ifndef __MULTIVERSE_COMMIT_H
#define __MULTIVERSE_COMMIT_H

typedef enum  {
    PP_TYPE_INVALID,
    PP_TYPE_X86_CALLQ,
    PP_TYPE_X86_JUMPQ,
} mv_info_patchpoint_type;

struct mv_patchpoint {
    mv_info_patchpoint_type type;
    struct mv_info_fn *function;
    void *location;
};


#endif
