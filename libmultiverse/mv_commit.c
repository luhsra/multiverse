#include <stdio.h>
#include "multiverse.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#include "mv_commit.h"

extern struct mv_info *mv_information;

static mv_value_t multiverse_var_read(struct mv_info_var *var) {
    mv_value_t ret;
    if (var->variable_width = sizeof(unsigned char)) {
        return *(unsigned char *)var->variable_location;
    } else if (var->variable_width = sizeof(unsigned short)) {
        return *(unsigned short *)var->variable_location;
    } else if (var->variable_width = sizeof(unsigned int)) {
        return *(unsigned int *)var->variable_location;
    }
    assert (0 && "Invalid width of multiverse variable. This should not happen");
}

typedef struct {
    int cache_size;
    void *cache[10];
} mv_select_ctx_t;

static mv_select_ctx_t *multiverse_select_start() {
    mv_select_ctx_t *ret = calloc(1, sizeof(mv_select_ctx_t));
    ret->cache_size = 10;
    return ret;
}


static void multiverse_select_protect(mv_select_ctx_t *ctx, void *addr) {
    (void) ctx;
    uintptr_t pagesize = sysconf(_SC_PAGESIZE);
    void *page = (void*)((uintptr_t) addr & ~(pagesize - 1));
    if (mprotect(page, pagesize, PROT_READ | PROT_EXEC)) {
        assert(0 && "mprotect should not fail");
    }
    // Flush instruction and data cache
    printf("protect %p\n", page);
}

static void multiverse_select_end(mv_select_ctx_t *ctx) {
    for (unsigned i = 0; i < ctx->cache_size; i++) {
        if (ctx->cache[i] != NULL) {
            multiverse_select_protect(ctx, ctx->cache[i]);
        }
    }
    free(ctx);
}



static void multiverse_select_unprotect(mv_select_ctx_t *ctx, void *addr) {
    uintptr_t pagesize = sysconf(_SC_PAGESIZE);
    void *page = (void*)((uintptr_t) addr & ~(pagesize - 1));
    // The unprotected_pages implements a LRU cache, were element 0 is
    // the most hottest one.
    for (unsigned i = 0; i < ctx->cache_size; i++) {
        if (ctx->cache[i] == page) {
            // Shift everthing one position behind
            for (unsigned j = i; j > 0; j--) {
                ctx->cache[j] = ctx->cache[j-1];
            }
            // printf("already unprotected %p\n", page);
            ctx->cache[0] = page;
            return;
        }
    }
    // Not yet unprotected
    if (mprotect(page, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC)) {
        perror("mprotect");
        printf("%p, %p\n", addr, page);
        assert(0 && "mprotect should not fail");
    }
    // Insert into the cache: shift everything one back.
    if (ctx->cache[ctx->cache_size - 1]) {
        multiverse_select_protect(ctx, ctx->cache[ctx->cache_size - 1]);
    }
    for (unsigned j = ctx->cache_size - 2; j > 0; j--) {
        ctx->cache[j] = ctx->cache[j-1];
    }
    ctx->cache[0] = page;
    printf("unprotected ");
    for (unsigned i = 0; i < ctx->cache_size && ctx->cache[i]; i++) {
        printf(" %p", ctx->cache[i]);
    }
    printf("\n");
}

static int callq_argument(void * callsite, void * callee) {
    return (uintptr_t)callee - ((uintptr_t) callsite + 5);
}

static int
multiverse_select_mvfn(mv_select_ctx_t *ctx,
                       struct mv_info_fn *fn,
                       struct mv_info_mvfn *mvfn) {
    assert(mvfn != NULL && "Reverting to general implementation not implemented yet");
    if (mvfn == fn->extra->active_mvfn) return 0;

    char *prologue = fn->function_body;
    multiverse_select_unprotect(ctx, prologue);
    multiverse_select_unprotect(ctx, prologue+5);

    for (unsigned i = 0; i < fn->extra->n_patchpoints; i++) {
        struct mv_patchpoint *pp = &fn->extra->patchpoints[i];
        unsigned char *location = pp->location;
        if (pp->type == PP_TYPE_INVALID) continue;
        if (!location) continue;
        multiverse_select_unprotect(ctx, location);
        multiverse_select_unprotect(ctx, location+5);

        if (pp->type == PP_TYPE_X86_CALLQ) {
            printf("patch %p: call %p\n", location, mvfn->function_body);
            location[0] = 0xe8;
        } else if (pp->type == PP_TYPE_X86_JUMPQ) {
            printf("patch %p: jump %p\n", location, mvfn->function_body);
            location[0] = 0xe9;
        }

        *((int *)&location[1]) = callq_argument(location, mvfn->function_body);
        __clear_cache(location, location+5);

    }

    fn->extra->active_mvfn = mvfn;

    return 1; // We changed this function
}

int __multiverse_commit_fn(mv_select_ctx_t *ctx, struct mv_info_fn *fn) {
    struct mv_info_mvfn *best_mvfn = NULL;

    for (unsigned f = 0; f < fn->n_mv_functions; f++) {
        struct mv_info_mvfn * mvfn = &fn->mv_functions[f];
        unsigned good = 1;
        for (unsigned a = 0; a < mvfn->n_assignments; a++) {
            struct mv_info_assignment * assign = &mvfn->assignments[a];
            mv_value_t cur = multiverse_var_read(assign->variable);
            if (cur > assign->upper_bound || cur < assign->lower_bound) good = 0;
        }
        if (good) {
            // Here we do possibliy override an already valid mvfn
            best_mvfn = mvfn;
        }
    }
    return multiverse_select_mvfn(ctx, fn, best_mvfn);
}

int multiverse_commit_info_fn(struct mv_info_fn *fn) {
    mv_select_ctx_t *ctx = multiverse_select_start();
    if (!ctx) return -1;
    int ret = __multiverse_commit_fn(ctx, fn);
    multiverse_select_end(ctx);

    return ret;
}

int multiverse_commit_fn(void *function_body) {
    struct mv_info_fn *fn = multiverse_info_fn(function_body);
    if (!fn) return -1;

    return multiverse_commit_info_fn(fn);
}

int multiverse_commit_var_info(struct mv_info_var *var) {
    int ret = 0;
    mv_select_ctx_t *ctx = multiverse_select_start();
    if (!ctx) return -1;
    for (unsigned f = 0; f < var->extra->n_functions; ++f) {
        int r = __multiverse_commit_fn(ctx, var->extra->functions[f]);
        if (r < 0) {
            ret = -1;
            break;
        }
        ret += r;
    }

    multiverse_select_end(ctx);

    return ret;
}

int multiverse_commit_var(void *variable_location) {
    struct mv_info_var *var = multiverse_info_var(variable_location);
    if (!var) return -1;
    return multiverse_commit_var_info(var);
}

int multiverse_commit() {
    int ret = 0;
    mv_select_ctx_t *ctx = multiverse_select_start();
    assert(ctx != NULL);
    for (struct mv_info *info = mv_information; info; info = info->next) {
        for (unsigned i = 0; i < info->n_functions; ++i) {
            int r = __multiverse_commit_fn(ctx, &info->functions[i]);
            if (r < 0) return -1; // FIXME: get a valid state after this
            ret += r;
        }
    }

    multiverse_select_end(ctx);

    return ret;
}
