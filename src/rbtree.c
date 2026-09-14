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

static bool is_red(const rb_node_t *node) {
    return node != NULL && node->color == RB_RED;
}

static void rotate_left(rbtree_t *t, rb_node_t *x) {
    rb_node_t *y = x->right;

    x->right = y->left;
    if (y->left != NULL) {
        y->left->parent = x;
    }

    y->parent = x->parent;
    if (x->parent == NULL) {
        t->root = y;
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
        x->parent->right = y;
    }

    y->left = x;
    x->parent = y;
}

static void rotate_right(rbtree_t *t, rb_node_t *x) {
    rb_node_t *y = x->left;

    x->left = y->right;
    if (y->right != NULL) {
        y->right->parent = x;
    }

    y->parent = x->parent;
    if (x->parent == NULL) {
        t->root = y;
    } else if (x == x->parent->right) {
        x->parent->right = y;
    } else {
        x->parent->left = y;
    }

    y->right = x;
    x->parent = y;
}

static void insert_fixup(rbtree_t *t, rb_node_t *z) {
    while (is_red(z->parent)) {    /* invariant: z is red; loop only continues while a red-red violation exists */
        if (z->parent == z->parent->parent->left) {
            rb_node_t *uncle = z->parent->parent->right;
            if (is_red(uncle)) {
                z->parent->color = RB_BLACK;
                uncle->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    rotate_left(t, z);
                }
                z->parent->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                rotate_right(t, z->parent->parent);
            }
        } else {
            rb_node_t *uncle = z->parent->parent->left;
            if (is_red(uncle)) {
                z->parent->color = RB_BLACK;
                uncle->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    rotate_right(t, z);
                }
                z->parent->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                rotate_left(t, z->parent->parent);
            }
        }
    }

    t->root->color = RB_BLACK;
}

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

    insert_fixup(t, node);

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

static rb_node_t *find_node(const rbtree_t *t, const char *key) {
    rb_node_t *current = t->root;

    while (current != NULL) {      /* invariant: current is the next node to compare against */
        int cmp = strcmp(key, current->key);
        if (cmp == 0) {
            return current;
        }
        current = (cmp < 0) ? current->left : current->right;
    }

    return NULL;
}

static rb_node_t *minimum(rb_node_t *node) {
    while (node->left != NULL) {   /* invariant: node is a lower bound on all keys seen so far */
        node = node->left;
    }
    return node;
}

static void transplant(rbtree_t *t, rb_node_t *u, rb_node_t *v) {
    if (u->parent == NULL) {
        t->root = v;
    } else if (u == u->parent->left) {
        u->parent->left = v;
    } else {
        u->parent->right = v;
    }
    if (v != NULL) {
        v->parent = u->parent;
    }
}

static void delete_fixup(rbtree_t *t, rb_node_t *x, rb_node_t *x_parent) {
    while (x != t->root && !is_red(x)) {   /* invariant: x is doubly-black; x_parent is x's real parent, valid even when x == NULL */
        if (x == x_parent->left) {
            rb_node_t *w = x_parent->right;
            if (is_red(w)) {
                w->color = RB_BLACK;
                x_parent->color = RB_RED;
                rotate_left(t, x_parent);
                w = x_parent->right;
            }
            if (!is_red(w->left) && !is_red(w->right)) {
                w->color = RB_RED;
                x = x_parent;
                x_parent = x->parent;
            } else {
                if (!is_red(w->right)) {
                    if (w->left != NULL) {
                        w->left->color = RB_BLACK;
                    }
                    w->color = RB_RED;
                    rotate_right(t, w);
                    w = x_parent->right;
                }
                w->color = x_parent->color;
                x_parent->color = RB_BLACK;
                if (w->right != NULL) {
                    w->right->color = RB_BLACK;
                }
                rotate_left(t, x_parent);
                x = t->root;
                x_parent = NULL;
            }
        } else {
            rb_node_t *w = x_parent->left;
            if (is_red(w)) {
                w->color = RB_BLACK;
                x_parent->color = RB_RED;
                rotate_right(t, x_parent);
                w = x_parent->left;
            }
            if (!is_red(w->right) && !is_red(w->left)) {
                w->color = RB_RED;
                x = x_parent;
                x_parent = x->parent;
            } else {
                if (!is_red(w->left)) {
                    if (w->right != NULL) {
                        w->right->color = RB_BLACK;
                    }
                    w->color = RB_RED;
                    rotate_left(t, w);
                    w = x_parent->left;
                }
                w->color = x_parent->color;
                x_parent->color = RB_BLACK;
                if (w->left != NULL) {
                    w->left->color = RB_BLACK;
                }
                rotate_right(t, x_parent);
                x = t->root;
                x_parent = NULL;
            }
        }
    }

    if (x != NULL) {
        x->color = RB_BLACK;
    }
}

int rb_delete(rbtree_t *t, const char *key) {
    rb_node_t *z = find_node(t, key);
    if (z == NULL) {
        return -1;
    }

    rb_node_t *y = z;
    rb_color_t y_original_color = y->color;
    rb_node_t *x;
    rb_node_t *x_parent;

    if (z->left == NULL) {
        x = z->right;
        x_parent = z->parent;
        transplant(t, z, z->right);
    } else if (z->right == NULL) {
        x = z->left;
        x_parent = z->parent;
        transplant(t, z, z->left);
    } else {
        y = minimum(z->right);
        y_original_color = y->color;
        x = y->right;
        if (y->parent == z) {
            x_parent = y;
        } else {
            x_parent = y->parent;
            transplant(t, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }
        transplant(t, z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }

    if (y_original_color == RB_BLACK) {
        delete_fixup(t, x, x_parent);
    }

    free(z->key);
    if (t->value_free != NULL) {
        t->value_free(z->value);
    }
    free(z);
    t->size--;
    return 0;
}

size_t rb_size(const rbtree_t *t) {
    return t->size;
}

static void foreach_node(const rb_node_t *node,
                          void (*fn)(const char *key, void *value, void *ctx),
                          void *ctx) {
    if (node == NULL) {
        return;
    }
    foreach_node(node->left, fn, ctx);
    fn(node->key, node->value, ctx);
    foreach_node(node->right, fn, ctx);
}

void rb_foreach(const rbtree_t *t,
                void (*fn)(const char *key, void *value, void *ctx),
                void *ctx) {
    foreach_node(t->root, fn, ctx);
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
