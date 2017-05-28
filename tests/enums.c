#include <stdio.h>
#include <assert.h>

#include "multiverse.h"
typedef enum {true, false} bool;

typedef enum {debug, notice, info, warning, error} state;


__attribute__((multiverse)) unsigned char byte_a;
__attribute__((multiverse)) bool conf_a;
__attribute__((multiverse)) bool conf_b;
__attribute__((multiverse)) bool conf_c;
__attribute__((multiverse)) state state_a;
__attribute__((multiverse)) state state_b;



int __attribute__((multiverse)) many_variants() {
    if (conf_a && conf_b && conf_c) {
        return state_a;
    }
    return state_b;
}

int __attribute__((multiverse)) integral_and_enum() {
    if (state_a == error && byte_a) return 0;
    return -1;
}


int main(int argc, char **argv)
{
    multiverse_init();

    multiverse_dump_info();

    // Functional Test
    for (conf_a = 0; conf_a <= 1; conf_a ++) {
        for (conf_b = 0; conf_b <= 1; conf_b ++) {
            for (conf_c = 0; conf_c <= 1; conf_c ++) {
                for (state_a = debug; state_a <= error; state_a ++) {
                    for (state_b = debug; state_b <= error; state_b ++) {
                        multiverse_commit_fn(&many_variants);
                        assert(multiverse_is_committed(&many_variants));

                        int x = many_variants();
                        if (conf_a && conf_b && conf_c) {
                            assert (x == state_a);
                        } else {
                            assert (x == state_b);
                        }
                    }
                }
            }
        }
    }

    for (byte_a = 0; byte_a <= 1; byte_a ++) {
        for (state_a = debug; state_a <= error; state_a ++) {
            multiverse_commit_fn(&integral_and_enum);
            assert(multiverse_is_committed(&integral_and_enum));

            int x = integral_and_enum();
            assert (x == ((state_a == error && byte_a) ? 0 : -1));
        }
    }
    return 0;
}
