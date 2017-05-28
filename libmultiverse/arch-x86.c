#include "mv_types.h"
#include "mv_string.h"
#include "multiverse.h"
#include "arch.h"
#include "platform.h"

void multiverse_arch_decode_function(struct mv_info_fn *fn,
                                     struct mv_patchpoint *pp) {
    pp->type     = PP_TYPE_X86_JUMPQ;
    pp->function = fn;
    pp->location = fn->function_body;
}

void multiverse_arch_decode_callsite(struct mv_info_fn *fn,
                                     void * addr,
                                     struct mv_patchpoint *info) {
    unsigned char *p = addr;
    void * callee = p + *(int*)(p + 1) + 5;
    /* 0xe8 */
    if (p[0] == 0xe8 && callee == fn->function_body) {
        info->type = PP_TYPE_X86_CALLQ;
        info->function = fn;
        info->location = p;
        return;
    }
    info->type = PP_TYPE_INVALID;
}

static int is_ret(char *addr) {
    //    c3: retq
    // f3 c3: repz retq
    return (memcmp(addr, "\xc3", 1) == 0)
        || (memcmp(addr, "\xf3\xc3", 2) == 0);
}

void multiverse_arch_decode_mvfn_body(void * addr,
                                      struct mv_info_mvfn_extra *info) {
    char *op = addr;
    // 31 c0: xor    %eax,%eax
    //    c3: retq
    if (memcmp(op, "\x31\xc0", 2) == 0 && is_ret(op + 2)) {
        // multiverse_os_print("eax = 0\n");
        info->type = MVFN_TYPE_CONSTANT;
        info->constant = 0;
    } else if (memcmp(op, "\xb8", 1) == 0 && is_ret(op + 5)) {
        info->type = MVFN_TYPE_CONSTANT;
        info->constant = *(uint32_t *)(op +1);
        // multiverse_os_print("eax = %d\n", info->constant);
    } else if (is_ret(addr)) {
        // multiverse_os_print("NOP\n");
        info->type = MVFN_TYPE_NOP;
    } else {
        info->type = MVFN_TYPE_NONE;
    }
}

static void insert_offset_argument(unsigned char * callsite, void * callee) {
    uint32_t offset = (uintptr_t)callee - ((uintptr_t) callsite + 5);
    *((uint32_t *)&callsite[1]) = offset;
}

void multiverse_arch_patchpoint_apply(struct mv_info_fn *fn,
                                      struct mv_info_mvfn *mvfn,
                                      struct mv_patchpoint *pp) {
    unsigned char *location = pp->location;
    // Select from original -> Swap out the current code
    if (fn->extra->active_mvfn == NULL) {
        memcpy(&pp->swapspace[0], location, 5);
    }

    // patch the code segment according to the patchpoint definition
    if (pp->type == PP_TYPE_X86_CALLQ) {
        // Oh, look. It has a very simple body!
        if (mvfn->extra && mvfn->extra->type == MVFN_TYPE_NOP) {
            // multiverse_os_print("patch %p: NOP\n", location);
            // 5 byte NOP instruction
            memcpy(location, "\x0F\x1F\x44\x00\x00", 5);
        } else if (mvfn->extra && mvfn->extra->type == MVFN_TYPE_CONSTANT) {
            // multiverse_os_print("patch %p: const = %d\n", location,
            //                     mvfn->extra->constant);
            location[0] = 0xb8; // mov $..., eax
            *(uint32_t *)(location + 1) = mvfn->extra->constant;
        } else {
            // multiverse_os_print("patch %p: call %p\n", location,
            //                     mvfn->function_body);
            location[0] = 0xe8;
            insert_offset_argument(location, mvfn->function_body);
        }
    } else if (pp->type == PP_TYPE_X86_JUMPQ) {
        // multiverse_os_print("patch %p: jump %p\n",
        //                     location, mvfn->function_body);
        location[0] = 0xe9;
        insert_offset_argument(location, mvfn->function_body);
    }

    // In all cases: Clear the cache afterwards.
    multiverse_os_clear_cache(location, 5);
}

void multiverse_arch_patchpoint_revert(struct mv_patchpoint *pp) {
    unsigned char *location = pp->location;
    // Revert to original state
    memcpy(pp->location, &pp->swapspace[0], 5);
    // multiverse_os_print("patch %p: original\n", pp->location);
    multiverse_os_clear_cache(location, 5);
}

void multiverse_arch_patchpoint_size(struct mv_patchpoint *pp,
                                     void **from,
                                     void**to) {
    *from = pp->location;
    *to = pp->location + 5;
}
