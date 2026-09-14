#include "rbtree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks_run = 0;
static int checks_failed = 0;

#define CHECK(cond) do { \
    checks_run++; \
    if (!(cond)) { \
        checks_failed++; \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

static int val_a = 1, val_b = 2, val_c = 3;

static void check_root_rotation(const char *label,
                                 const char *k1, void *v1,
                                 const char *k2, void *v2,
                                 const char *k3, void *v3) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    CHECK(rb_insert(t, k1, v1) == 0);
    CHECK(rb_insert(t, k2, v2) == 0);
    CHECK(rb_insert(t, k3, v3) == 0);

    CHECK(rb_validate(t) == 0);
    CHECK(rb_find(t, k1) == v1);
    CHECK(rb_find(t, k2) == v2);
    CHECK(rb_find(t, k3) == v3);

    rb_destroy(t);
    printf("checked: %s\n", label);
}

/* Every insertion sequence below was hand-traced against the CLRS-style
 * insert_fixup in src/rbtree.c to pin down the exact color/shape rb_delete
 * will see at the target key, so each row exercises one specific
 * delete-fixup situation rather than whatever a random sequence happens
 * to produce. */
typedef struct {
    const char *label;
    const char *insert_keys[7]; /* insertion order, NULL-terminated */
    const char *delete_key;
} delete_case_t;

static int dummy_value;

static const delete_case_t delete_cases[] = {
    { "red leaf (left child)",
      { "b", "a", "c", NULL }, "a" },
    { "red leaf (right child), mirror",
      { "b", "a", "c", NULL }, "c" },
    { "black leaf with red sibling (sibling on the right)",
      { "a", "b", "c", "d", "e", "f", NULL }, "a" },
    { "black leaf with red sibling (sibling on the left), mirror",
      { "f", "e", "d", "c", "b", "a", NULL }, "f" },
    { "node with two children",
      { "a", "b", "c", "d", "e", "f", NULL }, "d" },
    { "node with two children, mirror",
      { "f", "e", "d", "c", "b", "a", NULL }, "c" },
    { "root deletion (single-node tree)",
      { "m", NULL }, "m" },
    { "black node with exactly one red child (child on the right)",
      { "b", "a", "d", "f", NULL }, "d" },
    { "black node with exactly one red child (child on the left), mirror",
      { "e", "f", "c", "a", NULL }, "c" },
};

static void run_delete_case(const delete_case_t *tc) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    size_t n = 0;
    for (size_t i = 0; tc->insert_keys[i] != NULL; i++) {      /* invariant: n keys inserted so far */
        CHECK(rb_insert(t, tc->insert_keys[i], &dummy_value) == 0);
        n++;
    }
    CHECK(rb_validate(t) == 0);
    CHECK(rb_size(t) == n);

    CHECK(rb_delete(t, tc->delete_key) == 0);

    CHECK(rb_validate(t) == 0);
    CHECK(rb_size(t) == n - 1);
    CHECK(rb_find(t, tc->delete_key) == NULL);
    for (size_t i = 0; tc->insert_keys[i] != NULL; i++) {      /* invariant: every surviving key still resolves */
        if (strcmp(tc->insert_keys[i], tc->delete_key) != 0) {
            CHECK(rb_find(t, tc->insert_keys[i]) == &dummy_value);
        }
    }

    rb_destroy(t);
    printf("checked: %s\n", tc->label);
}

static void check_destroy_empty_and_null(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);
    CHECK(rb_size(t) == 0);
    rb_destroy(t);

    rb_destroy(NULL);   /* NULL-safe per the header contract */

    printf("checked: destroy empty tree and NULL tree\n");
}

static int destroyed_count = 0;

static void counting_free(void *value) {
    destroyed_count++;
    free(value);
}

static void check_destroy_frees_values(void) {
    rbtree_t *t = rb_create(counting_free);
    CHECK(t != NULL);

    const char *keys[] = { "x", "y", "z" };
    size_t n = sizeof(keys) / sizeof(keys[0]);
    for (size_t i = 0; i < n; i++) {      /* invariant: i values inserted so far, none yet deleted */
        int *value = malloc(sizeof(int));
        CHECK(value != NULL);
        *value = (int)i;
        CHECK(rb_insert(t, keys[i], value) == 0);
    }

    destroyed_count = 0;
    rb_destroy(t);
    CHECK(destroyed_count == (int)n);

    printf("checked: destroy calls value_free exactly once per remaining value\n");
}

int main(void) {
    check_root_rotation("line, left branch (rotate_right at root)",
                         "c", &val_c, "b", &val_b, "a", &val_a);
    check_root_rotation("line, mirror (rotate_left at root)",
                         "a", &val_a, "b", &val_b, "c", &val_c);
    check_root_rotation("triangle, left branch (rotate_left then rotate_right at root)",
                         "c", &val_c, "a", &val_a, "b", &val_b);
    check_root_rotation("triangle, mirror (rotate_right then rotate_left at root)",
                         "a", &val_a, "c", &val_c, "b", &val_b);

    size_t n_delete_cases = sizeof(delete_cases) / sizeof(delete_cases[0]);
    for (size_t i = 0; i < n_delete_cases; i++) {      /* invariant: cases [0,i) already run */
        run_delete_case(&delete_cases[i]);
    }

    check_destroy_empty_and_null();
    check_destroy_frees_values();

    if (checks_failed == 0) {
        printf("PASS: %d checks run, 0 failed\n", checks_run);
        return 0;
    }
    printf("FAIL: %d checks run, %d failed\n", checks_run, checks_failed);
    return 1;
}
