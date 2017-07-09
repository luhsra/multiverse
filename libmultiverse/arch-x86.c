#include "mv_string.h"
#include "multiverse.h"
#include "arch.h"
#include "platform.h"

void multiverse_arch_decode_function(struct mv_info_fn *fn,
                                     struct mv_patchpoint *pp) {
    pp->type     = PP_TYPE_X86_JUMP;
    pp->function = fn;
    pp->location = fn->function_body;
}

void multiverse_arch_decode_callsite(struct mv_info_fn *fn,
                                     void * addr,
                                     struct mv_patchpoint *info) {
    unsigned char *p = addr;
    if (p[0] == 0xe8) {
        // normal call
        void * callee = p + *(int*)(p + 1) + 5;
        if (callee == fn->function_body) {
            info->type = PP_TYPE_X86_CALL;
            info->function = fn;
            info->location = p;
            return;
        }
    } else if (p[0] == 0xff && p[1] == 0x15) {
        // indirect call (function pointer)
        void * callee_p = p + *(int*)(p + 2) + 6;
        if (callee_p == fn->function_body) {
            info->type = PP_TYPE_X86_CALL_INDIRECT;
            info->function = fn;
            info->location = p;
            return;
        }
    }
    info->type = PP_TYPE_INVALID;
}

static int is_ret(char *addr) {
    //    c3: retq
    // f3 c3: repz retq
    return (memcmp(addr, "\xc3", 1) == 0)
        || (memcmp(addr, "\xf3\xc3", 2) == 0);
}

static int location_len(mv_info_patchpoint_type type) {
    if (type == PP_TYPE_X86_CALL_INDIRECT) {
        return 6;
    } else {
        return 5;
    }
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
        memcpy(&pp->swapspace[0], location, location_len(pp->type));
    }

    // patch the code segment according to the patchpoint definition
    if (pp->type == PP_TYPE_X86_CALL || pp->type == PP_TYPE_X86_CALL_INDIRECT) {
        // Oh, look. It has a very simple body!
        if (mvfn->extra && mvfn->extra->type == MVFN_TYPE_NOP) {
            if (pp->type == PP_TYPE_X86_CALL_INDIRECT) {
                // 6 byte NOP instruction
                memcpy(location, "\x66\x0F\x1F\x44\x00\x00", 6);
            } else {
                // 5 byte NOP instruction
                memcpy(location, "\x0F\x1F\x44\x00\x00", 5);
            }
        } else if (mvfn->extra && mvfn->extra->type == MVFN_TYPE_CONSTANT) {
            location[0] = 0xb8; // mov $..., eax
            *(uint32_t *)(location + 1) = mvfn->extra->constant;
            if (pp->type == PP_TYPE_X86_CALL_INDIRECT)
                location[5] = '\x90'; // insert trailing NOP
        } else {
            location[0] = 0xe8;
            insert_offset_argument(location, mvfn->function_body);
            if (pp->type == PP_TYPE_X86_CALL_INDIRECT)
                location[5] = '\x90'; // insert trailing NOP
        }
    } else if (pp->type == PP_TYPE_X86_JUMP) {
        location[0] = 0xe9;
        insert_offset_argument(location, mvfn->function_body);
    }

    // In all cases: Clear the cache afterwards.
    multiverse_os_clear_cache(location, location_len(pp->type));
}

void multiverse_arch_patchpoint_revert(struct mv_patchpoint *pp) {
    unsigned char *location = pp->location;
    int size = location_len(pp->type);
    // Revert to original state
    memcpy(pp->location, &pp->swapspace[0], size);
    multiverse_os_clear_cache(location, size);
}

void multiverse_arch_patchpoint_size(struct mv_patchpoint *pp,
                                     void **from,
                                     void**to) {
    *from = pp->location;
    *to = pp->location + location_len(pp->type);
}
