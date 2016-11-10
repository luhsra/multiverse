#ifndef __GCC_PLUGIN_MULTIVERSE_H
#define __GCC_PLUGIN_MULTIVERSE_H

#ifdef DEBUG
#define debug_print(args...) fprintf(stderr, args)
#else
#define debug_print(args...) do {} while(0)
#endif

#endif
