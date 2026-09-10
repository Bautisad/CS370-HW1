#include "rbtree.h"
#include <stdio.h>

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

int main(void) {
    check_root_rotation("line, left branch (rotate_right at root)",
                         "c", &val_c, "b", &val_b, "a", &val_a);
    check_root_rotation("line, mirror (rotate_left at root)",
                         "a", &val_a, "b", &val_b, "c", &val_c);
    check_root_rotation("triangle, left branch (rotate_left then rotate_right at root)",
                         "c", &val_c, "a", &val_a, "b", &val_b);
    check_root_rotation("triangle, mirror (rotate_right then rotate_left at root)",
                         "a", &val_a, "c", &val_c, "b", &val_b);

    if (checks_failed == 0) {
        printf("PASS: %d checks run, 0 failed\n", checks_run);
        return 0;
    }
    printf("FAIL: %d checks run, %d failed\n", checks_run, checks_failed);
    return 1;
}
