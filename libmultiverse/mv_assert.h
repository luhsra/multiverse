#ifndef __MULTIVERSE_MV_ASSERT_H
#define __MULTIVERSE_MV_ASSERT_H

#ifdef MULTIVERSE_KERNELSPACE

#include <linux/bug.h>
#define MV_ASSERT(condition) WARN_ON(!(condition))

#else

#include <assert.h>
#define MV_ASSERT(condition) assert(condition)

#endif

#endif
