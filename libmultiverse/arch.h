#ifndef __MULTIVERSE_ARCH_H
#define __MULTIVERSE_ARCH_H

#include "mv_commit.h"
struct mv_info_mvfn;
struct mv_info_mvfn_extra;

/**
  @brief Decode callsite and fill patchpoint info

  This architecture specfic function decodes the callsite at addr and
  fills in the patchpoint information. On success the patchpoint type
  is != PP_TYPE_INVALID.
*/
void multiverse_arch_decode_callsite(struct mv_info_fn *fn, void *callsite,
                                      struct mv_patchpoint *pp);

/**
  @brief decode mvfn function body

  If a multiverse function body does nothing, or only returns an
  constant value, we can further optimized the patchedb callsites
*/

void multiverse_arch_decode_mvfn_body(void * addr,
                                      struct mv_info_mvfn_extra *info);

/**
  @brief applies the mvfn to the patchpoint
*/
void multiverse_arch_patchpoint_apply(struct mv_info_fn *fn,
                                      struct mv_info_mvfn *mvfn,
                                      struct mv_patchpoint *pp);

/**
   @brief brings the code to the original form
*/
void multiverse_arch_patchpoint_revert(struct mv_patchpoint *pp);

#endif
