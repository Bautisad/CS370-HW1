#include "rbtree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define KEY_BUF_LEN  16
#define KEYSPACE_CAP 2000

typedef struct {
    char key[KEY_BUF_LEN];
    int value;
} model_entry_t;

typedef struct {
    model_entry_t *entries;
    size_t count;
    size_t capacity;
} model_t;

/* Lower-bound binary search over model->entries[0, count).
 * Returns true with *idx_out set to the exact match, or false with
 * *idx_out set to the index at which key would be inserted. */
static bool model_bsearch(const model_t *m, const char *key, size_t *idx_out) {
    size_t lo = 0, hi = m->count;
    while (lo < hi) {   /* invariant: an exact match, if any, lies in [lo, hi) */
        size_t mid = lo + (hi - lo) / 2;
        int cmp = strcmp(key, m->entries[mid].key);
        if (cmp == 0) {
            *idx_out = mid;
            return true;
        }
        if (cmp < 0) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    *idx_out = lo;
    return false;
}

static void model_insert(model_t *m, const char *key, int value) {
    size_t idx;
    if (model_bsearch(m, key, &idx)) {
        m->entries[idx].value = value;
        return;
    }

    if (m->count >= m->capacity) {
        fprintf(stderr, "model overflow: count %zu >= capacity %zu\n", m->count, m->capacity);
        exit(1);
    }

    size_t tail = m->count - idx;
    if (tail > 0) {
        memmove(&m->entries[idx + 1], &m->entries[idx], tail * sizeof(model_entry_t));
    }
    snprintf(m->entries[idx].key, KEY_BUF_LEN, "%s", key);
    m->entries[idx].value = value;
    m->count++;
}

static bool model_delete(model_t *m, const char *key) {
    size_t idx;
    if (!model_bsearch(m, key, &idx)) {
        return false;
    }

    size_t tail = m->count - idx - 1;
    if (tail > 0) {
        memmove(&m->entries[idx], &m->entries[idx + 1], tail * sizeof(model_entry_t));
    }
    m->count--;
    return true;
}

static bool model_find(const model_t *m, const char *key, int *value_out) {
    size_t idx;
    if (!model_bsearch(m, key, &idx)) {
        return false;
    }
    *value_out = m->entries[idx].value;
    return true;
}

/* Half the time pick a key known to exist (exercises the "found" path
 * reliably); otherwise pick a fully random key that may or may not exist. */
static void pick_lookup_key(const model_t *m, int keyspace, char *buf, size_t buflen) {
    if (m->count > 0 && rand() % 2 == 0) {
        size_t idx = (size_t)rand() % m->count;
        snprintf(buf, buflen, "%s", m->entries[idx].key);
    } else {
        int keynum = rand() % keyspace;
        snprintf(buf, buflen, "%d", keynum);
    }
}

typedef struct {
    const model_t *model;
    size_t cursor;
    bool ok;
} crosscheck_ctx_t;

static void crosscheck_fn(const char *key, void *value, void *ctx_raw) {
    crosscheck_ctx_t *ctx = ctx_raw;
    if (!ctx->ok) {
        return;
    }
    if (ctx->cursor >= ctx->model->count) {
        fprintf(stderr, "crosscheck: tree has extra key \"%s\" beyond model's %zu entries\n",
                key, ctx->model->count);
        ctx->ok = false;
        return;
    }

    const model_entry_t *expected = &ctx->model->entries[ctx->cursor];
    if (strcmp(key, expected->key) != 0) {
        fprintf(stderr, "crosscheck: key mismatch at position %zu: tree=\"%s\" model=\"%s\"\n",
                ctx->cursor, key, expected->key);
        ctx->ok = false;
        return;
    }
    if (*(int *)value != expected->value) {
        fprintf(stderr, "crosscheck: value mismatch for key \"%s\": tree=%d model=%d\n",
                key, *(int *)value, expected->value);
        ctx->ok = false;
        return;
    }
    ctx->cursor++;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <iterations> [seed]\n", argv[0]);
        return 1;
    }

    int iterations = atoi(argv[1]);

    unsigned seed = (argc >= 3) ? (unsigned)strtoul(argv[2], NULL, 10) : (unsigned)time(NULL);
    srand(seed);
    printf("seed: %u\n", seed);

    int keyspace = iterations > 0 ? iterations : 1;
    if (keyspace > KEYSPACE_CAP) {
        keyspace = KEYSPACE_CAP;
    }

    model_t model = { .entries = NULL, .count = 0, .capacity = (size_t)keyspace };
    model.entries = malloc(model.capacity * sizeof(model_entry_t));
    if (model.entries == NULL) {
        fprintf(stderr, "model allocation failed\n");
        return 1;
    }

    rbtree_t *t = rb_create(free);
    if (t == NULL) {
        fprintf(stderr, "rb_create failed\n");
        free(model.entries);
        return 1;
    }

    size_t ops_performed = 0;
    int next_value = 0;
    bool failed = false;

    for (int i = 0; i < iterations && !failed; i++) {   /* invariant: model mirrors the tree's exact key set through iteration i */
        int op = rand() % 3;
        char key[KEY_BUF_LEN];

        if (op == 0) {
            int keynum = rand() % keyspace;
            snprintf(key, sizeof(key), "%d", keynum);

            int *value = malloc(sizeof(int));
            if (value == NULL) {
                continue;
            }
            *value = next_value++;

            int rc = rb_insert(t, key, value);
            if (rc == 0) {
                model_insert(&model, key, *value);
            } else {
                free(value);
            }
            ops_performed++;
        } else if (op == 1) {
            pick_lookup_key(&model, keyspace, key, sizeof(key));

            int expected_value = 0;
            bool expect_present = model_find(&model, key, &expected_value);
            void *got = rb_find(t, key);

            if (expect_present && got == NULL) {
                fprintf(stderr, "FUZZ FAILURE: find(\"%s\") expected present, got NULL (seed %u)\n",
                        key, seed);
                failed = true;
            } else if (expect_present && *(int *)got != expected_value) {
                fprintf(stderr,
                        "FUZZ FAILURE: find(\"%s\") value mismatch: expected %d, got %d (seed %u)\n",
                        key, expected_value, *(int *)got, seed);
                failed = true;
            } else if (!expect_present && got != NULL) {
                fprintf(stderr, "FUZZ FAILURE: find(\"%s\") expected absent, got non-NULL (seed %u)\n",
                        key, seed);
                failed = true;
            }
            ops_performed++;
        } else {
            pick_lookup_key(&model, keyspace, key, sizeof(key));

            int rc = rb_delete(t, key);
            bool was_present = model_delete(&model, key);
            if ((rc == 0) != was_present) {
                fprintf(stderr,
                        "FUZZ FAILURE: delete(\"%s\") rc=%d but model says present=%d (seed %u)\n",
                        key, rc, was_present, seed);
                failed = true;
            }
            ops_performed++;
        }

        if (!failed && ops_performed % 100 == 0) {
            if (rb_validate(t) != 0) {
                fprintf(stderr, "FUZZ FAILURE: rb_validate failed after %zu ops (seed %u)\n",
                        ops_performed, seed);
                failed = true;
            } else if (rb_size(t) != model.count) {
                fprintf(stderr,
                        "FUZZ FAILURE: size mismatch after %zu ops: tree=%zu model=%zu (seed %u)\n",
                        ops_performed, rb_size(t), model.count, seed);
                failed = true;
            }
        }
    }

    if (!failed) {
        if (rb_validate(t) != 0) {
            fprintf(stderr, "FUZZ FAILURE: final rb_validate failed (seed %u)\n", seed);
            failed = true;
        } else if (rb_size(t) != model.count) {
            fprintf(stderr, "FUZZ FAILURE: final size mismatch: tree=%zu model=%zu (seed %u)\n",
                    rb_size(t), model.count, seed);
            failed = true;
        }
    }

    if (!failed) {
        crosscheck_ctx_t ctx = { .model = &model, .cursor = 0, .ok = true };
        rb_foreach(t, crosscheck_fn, &ctx);
        if (!ctx.ok) {
            failed = true;
        } else if (ctx.cursor != model.count) {
            fprintf(stderr, "FUZZ FAILURE: tree yielded only %zu of %zu expected keys (seed %u)\n",
                    ctx.cursor, model.count, seed);
            failed = true;
        }
    }

    rb_destroy(t);
    free(model.entries);

    if (failed) {
        printf("validate: 1 (FAIL)\n");
        return 1;
    }

    printf("ops: %zu, final size: %zu\n", ops_performed, model.count);
    printf("validate: 0 (PASS)\n");
    return 0;
}
