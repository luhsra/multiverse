#include "memory.h"
#include <stdint.h>
#include <memory.h>
#include "multiverse.h"
#include "arch.h"
#include "platform.h"

void multiverse_arch_decode_function(struct mv_info_fn *fn,
                                     struct mv_patchpoint *pp) {
    pp->type     = PP_TYPE_ARM_B;
    pp->function = fn;
    pp->location = fn->function_body;
}

void multiverse_arch_decode_callsite(struct mv_info_fn *fn,
                                     void * addr,
                                     struct mv_patchpoint *info) {
    unsigned char *p = addr;
    unsigned char instr = 0x0F & p[3];
    // TODO identify callee
    if (instr == 0b1011 /*&& callee == fn->function_body*/) {
        info->type = PP_TYPE_ARM_BL;
        info->function = fn;
        info->location = p;
        return;
    }
    info->type = PP_TYPE_INVALID;
}

void multiverse_arch_decode_mvfn_body(void * addr,
                                      struct mv_info_mvfn_extra *info) {
    /* NOP */
}

static void insert_offset_argument(unsigned char * callsite, void * callee) {
    uint32_t offset = (((uintptr_t)callee - 8) - ((uintptr_t) callsite)) >> 2;
    callsite[2] = (offset >> 16) & 0xff;
    callsite[1] = (offset >> 8) & 0xff;
    callsite[0] = offset & 0xff;
}

void multiverse_arch_patchpoint_apply(struct mv_info_fn *fn,
                                      struct mv_info_mvfn *mvfn,
                                      struct mv_patchpoint *pp) {
    unsigned char *location = pp->location;
    // Select from original -> Swap out the current code
    if (fn->extra->active_mvfn == NULL) {
        memcpy(&pp->swapspace[0], location, 4);
    }

    // patch the code segment according to the patchpoint definition
    if (pp->type == PP_TYPE_ARM_BL) {
        // printf("patch %p: call %p\n", location, mvfn->function_body);
        // printf("  before: %p\n", *((uint32_t*)location));
        location[3] = 0b11101011; // TODO not always always-condition
        insert_offset_argument(location, mvfn->function_body);
        // printf("  after:  %p\n", *((uint32_t*)location));
    } else if (pp->type == PP_TYPE_ARM_B) {
        // printf("patch %p: jump %p\n", location, mvfn->function_body);
        // printf("  before: %p\n", *((uint32_t*)location));
        location[3] = 0b11101010; // TODO not always always-condition
        insert_offset_argument(location, mvfn->function_body);
        // printf("  after:  %p\n", *((uint32_t*)location));
    }

    // In all cases: Clear the cache afterwards.
    multiverse_os_clear_cache(location, 4);
}

void multiverse_arch_patchpoint_revert(struct mv_patchpoint *pp) {
    unsigned char *location = pp->location;
    // Revert to original state
    memcpy(pp->location, &pp->swapspace[0], 4);
    // printf("patch %p: original\n", pp->location);
    multiverse_os_clear_cache(location, 4);
}

void multiverse_arch_patchpoint_size(struct mv_patchpoint *pp,
                                     void **from,
                                     void**to) {
    *from = pp->location;
    *to = pp->location + 4;
}
