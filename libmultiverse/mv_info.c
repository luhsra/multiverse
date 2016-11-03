#include <stdio.h>
#include "mv_info.h"


void __multiverse_init(struct mv_info *info) {
    fprintf(stderr, "got mv_info %p, version: %d\n", info, info->version);
}
