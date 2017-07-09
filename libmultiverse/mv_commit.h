#ifndef __MULTIVERSE_COMMIT_H
#define __MULTIVERSE_COMMIT_H

typedef enum  {
    PP_TYPE_INVALID,
    PP_TYPE_X86_CALL,
    PP_TYPE_X86_CALL_INDIRECT,
    PP_TYPE_X86_JUMP,
} mv_info_patchpoint_type;

struct mv_patchpoint {
    mv_info_patchpoint_type type;
    struct mv_info_fn *function;
    void *location;

    // Here we swap in the code, we overwrite
    unsigned char swapspace[6];
};


#endif
