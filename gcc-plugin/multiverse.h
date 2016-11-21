#ifndef __GCC_PLUGIN_MULTIVERSE_H
#define __GCC_PLUGIN_MULTIVERSE_H

#include <stdint.h>
#include "gcc-common.h"

/* The multiverse generator is used to capture all dimensions
 * (variables) and their values for a specific multiverse function.
 * The variables, and the values are scored and sorted according to
 * their score. One can give a maximal numer of elements the generator
 * should emit. If a variable assignment never changes during the
 * generated sequence, the dimension is not multiversed. */
struct mv_variant_generator {
    struct dimension_value {
        tree variable;
        const char *value_label;
        unsigned HOST_WIDE_INT value;
        unsigned score;
    };
    struct dimension {
        tree variable;
        unsigned score;
        std::vector<dimension_value> values;
    };

    typedef std::vector<dimension_value> variant;

private:
    std::vector<dimension> dimensions;
    std::vector<unsigned>  state;
    unsigned element_count;
    unsigned maximal_elements;
    unsigned skip_dimensions;

public:
    void add_dimension(tree variable, unsigned score = 0);
    void add_dimension_value(tree variable, const char *label,
                             unsigned HOST_WIDE_INT value, unsigned score = 0);
    void start(unsigned maximal_elements = -1);
    bool end_p();
    variant next();
};

#endif
