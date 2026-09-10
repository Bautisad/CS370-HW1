#include "rbtree.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <iterations>\n", argv[0]);
        return 1;
    }

    int iterations = atoi(argv[1]);

    unsigned seed = (unsigned)time(NULL);
    srand(seed);
    printf("seed: %u\n", seed);

    rbtree_t *t = rb_create(free);
    if (t == NULL) {
        fprintf(stderr, "rb_create failed\n");
        return 1;
    }

    int keyspace = iterations > 0 ? iterations : 1;
    for (int i = 0; i < iterations; i++) {
        int keynum = rand() % keyspace;
        char key[16];
        snprintf(key, sizeof(key), "%d", keynum);

        int *value = malloc(sizeof(int));
        if (value == NULL) {
            continue;
        }
        *value = keynum;

        if (rb_insert(t, key, value) != 0) {
            free(value);
        }
    }

    int result = rb_validate(t);
    printf("validate: %d (%s)\n", result, result == 0 ? "PASS" : "FAIL");

    rb_destroy(t);

    return result == 0 ? 0 : 1;
}
