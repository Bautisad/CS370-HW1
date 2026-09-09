#include "rbtree.h"
#include <stdlib.h>
#include <string.h>

typedef enum { RB_BLACK, RB_RED } rb_color_t;

typedef struct rb_node {
    char *key;
    void *value;
    struct rb_node *left;
    struct rb_node *right;
    struct rb_node *parent;
    rb_color_t color;
} rb_node_t;

struct rbtree {
    rb_node_t *root;
    size_t size;
    rb_value_free_fn value_free;
};

rbtree_t *rb_create(rb_value_free_fn value_free) {
    rbtree_t *t = malloc(sizeof(*t));
    if (t == NULL) {
        return NULL;
    }
    t->root = NULL;
    t->size = 0;
    t->value_free = value_free;
    return t;
}

int rb_insert(rbtree_t *t, const char *key, void *value) {
    rb_node_t *parent = NULL;
    rb_node_t *current = t->root;
    int cmp = 0;
    rb_node_t *node = NULL;
    char *keycopy = NULL;
    int rc = 0;

    while (current != NULL) {      /* invariant: parent trails one step behind current */
        cmp = strcmp(key, current->key);
        if (cmp == 0) {
            if (t->value_free != NULL) {
                t->value_free(current->value);
            }
            current->value = value;
            return 0;
        }
        parent = current;
        current = (cmp < 0) ? current->left : current->right;
    }

    node = malloc(sizeof(*node));
    if (node == NULL) {
        rc = -1;
        goto cleanup;
    }

    size_t keylen = strlen(key) + 1;
    keycopy = malloc(keylen);
    if (keycopy == NULL) {
        rc = -1;
        goto cleanup;
    }
    memcpy(keycopy, key, keylen);

    node->key = keycopy;
    node->value = value;
    node->left = NULL;
    node->right = NULL;
    node->parent = parent;
    node->color = RB_RED;

    if (parent == NULL) {
        t->root = node;
    } else if (cmp < 0) {
        parent->left = node;
    } else {
        parent->right = node;
    }

    t->size++;
    return 0;

cleanup:
    free(node);
    free(keycopy);
    return rc;
}

void *rb_find(const rbtree_t *t, const char *key) {
    rb_node_t *current = t->root;

    while (current != NULL) {      /* invariant: current is the next node to compare against */
        int cmp = strcmp(key, current->key);
        if (cmp == 0) {
            return current->value;
        }
        current = (cmp < 0) ? current->left : current->right;
    }

    return NULL;
}

static bool is_red(const rb_node_t *node) {
    return node != NULL && node->color == RB_RED;
}

static int check_black_height(const rb_node_t *node) {
    if (node == NULL) {
        return 0;   /* NIL leaves are black by convention; contribute no additional black count */
    }

    if (node->color == RB_RED && (is_red(node->left) || is_red(node->right))) {
        return -1;  /* red-red violation at this node */
    }

    int left_height = check_black_height(node->left);
    if (left_height < 0) {
        return -1;
    }

    int right_height = check_black_height(node->right);
    if (right_height < 0) {
        return -1;
    }

    if (left_height != right_height) {
        return -1;  /* unequal black-height between the two subtrees */
    }

    return left_height + (node->color == RB_BLACK ? 1 : 0);
}

typedef struct {
    const char *last_key;
    bool have_last;
    size_t count;
    bool ok;
} inorder_ctx_t;

static void check_inorder(const rb_node_t *node, inorder_ctx_t *ctx) {
    if (node == NULL || !ctx->ok) {
        return;   /* nothing left to check once we hit a leaf or already found a violation */
    }

    check_inorder(node->left, ctx);
    if (!ctx->ok) {
        return;
    }

    if (ctx->have_last && strcmp(ctx->last_key, node->key) >= 0) {
        ctx->ok = false;
        return;
    }

    ctx->last_key = node->key;
    ctx->have_last = true;
    ctx->count++;

    check_inorder(node->right, ctx);
}

int rb_validate(const rbtree_t *t) {
    if (t->root != NULL && t->root->color != RB_BLACK) {
        return 1;
    }

    if (check_black_height(t->root) < 0) {
        return 1;
    }

    inorder_ctx_t ctx = { .last_key = NULL, .have_last = false, .count = 0, .ok = true };
    check_inorder(t->root, &ctx);
    if (!ctx.ok) {
        return 1;
    }

    if (ctx.count != t->size) {
        return 1;
    }

    return 0;
}

static void destroy_node(rb_node_t *node, rb_value_free_fn value_free) {
    if (node == NULL) {
        return;
    }
    destroy_node(node->left, value_free);
    destroy_node(node->right, value_free);
    if (value_free != NULL) {
        value_free(node->value);
    }
    free(node->key);
    free(node);
}

void rb_destroy(rbtree_t *t) {
    if (t == NULL) {
        return;
    }
    destroy_node(t->root, t->value_free);
    free(t);
}
